#include "fisx_xrfconfig.h"
#include "fisx_simpleini.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

XRFConfig::XRFConfig()
{
    this->setGeometry(45.0, 45.0, 90.);
}

void XRFConfig::setGeometry(const double & alphaIn, const double & alphaOut, const double & scatteringAngle)
{
    this->alphaIn = alphaIn;
    this->alphaOut = alphaOut;
    this->scatteringAngle = scatteringAngle;
}

void XRFConfig::readConfigurationFromFile(const std::string & fileName)
{
    SimpleIni iniFile = SimpleIni(fileName);
    std::map<std::string, std::string> sectionContents;
    std::string content;
    std::map<std::string, std::vector<double> > mapDoubles;
    std::map<std::string, std::vector<std::string> > mapStrings;
    std::vector<std::string> stringVector;
    std::vector<double> doubleVector;
    std::vector<double>::size_type iDoubleVector;
    std::vector<int> intVector, flagVector;
    std::vector<int>::size_type iIntVector;
    Material material;
    bool fisxFile;
    long counter;

    // find out if it is a fix or a PyMca configuration file
    sectionContents.clear();
    sectionContents = iniFile.readSection("fisx", false);
    fisxFile = true;
    if(!sectionContents.size())
    {
        fisxFile = false;
    }
    if (fisxFile)
    {
        std::cout << "Not implemented" << std::endl;
        return;
    }
    // Assume is a PyMca generated file.
    // TODO: Still to find out if it is a fir output file or a configuration file
    // Assume it is configuration file.
    // In case of fit file, the configuration is under [result.config]
    // GET BEAM
    sectionContents.clear();
    sectionContents = iniFile.readSection("fit", false);
    if(!sectionContents.size())
    {
        sectionContents = iniFile.readSection("result.config", false);
        if(!sectionContents.size())
        {
            throw std::invalid_argument("File not recognized as a fisx or PyMca configuration file.");
        }
        std::cout << "fit result file" << std::endl;
    }
    else
    {
        // In case of PyMca.ini file, the configuration is under [Fit.Configuration]
        ;
    }
    mapDoubles.clear();
    content = sectionContents["energy"];
    iniFile.parseStringAsMultipleValues(content, mapDoubles["energy"], -666.0);
    content = sectionContents["energyweight"];
    iniFile.parseStringAsMultipleValues(content, mapDoubles["weight"], -1.0);
    content = sectionContents["energyscatter"];
    iniFile.parseStringAsMultipleValues(content, intVector, -1);
    content = sectionContents["energyflag"];
    iniFile.parseStringAsMultipleValues(content, flagVector, 0);

    /*
    std::cout << "Passed" << std::endl;
    std::cout << "Energy size = " << mapDoubles["energy"].size() << std::endl;
    std::cout << "weight size = " << mapDoubles["weight"].size() << std::endl;
    std::cout << "scatter = " << intVector.size() << std::endl;
    std::cout << "falg = " << flagVector.size() << std::endl;
    */

    if (mapDoubles["weight"].size() == 0)
    {
        mapDoubles["weight"].resize(mapDoubles["energy"].size());
        std::fill(mapDoubles["weight"].begin(), mapDoubles["weight"].end(), 1.0);
    }
    if (intVector.size() == 0)
    {
        intVector.resize(mapDoubles["energy"].size());
        std::fill(intVector.begin(), intVector.end(), 1.0);
    }
    if (flagVector.size() == 0)
    {
        flagVector.resize(mapDoubles["energy"].size());
        std::fill(flagVector.begin(), flagVector.end(), 1.0);
    }

    counter = 0;
    iIntVector = flagVector.size();
    while(iIntVector > 0)
    {
        iIntVector--;
        if ((flagVector[iIntVector] > 0) && (mapDoubles["energy"][iIntVector] != -666.0))
        {
            if(mapDoubles["energy"][iIntVector] <= 0.0)
            {
                throw std::invalid_argument("Negative excitation beam photon energy");
            }
            if(mapDoubles["weight"][iIntVector] <= 0.0)
            {
                throw std::invalid_argument("Negative excitation beam photon weight");
            }
            if(intVector[iIntVector] < 0)
            {
                std::cout << "WARNING: " << "Negative characteristic flag. ";
                std::cout << "Assuming not a characteristic photon energy." << std::endl;
                intVector[iIntVector] = 0;
            }
            counter++;
        }
        else
        {
            // index not to be considered
            mapDoubles["energy"].erase(mapDoubles["energy"].begin() + iIntVector);
            mapDoubles["weight"].erase(mapDoubles["weight"].begin() + iIntVector);
            intVector.erase(intVector.begin() + iIntVector);
        }
    }
    this->setBeam(mapDoubles["energy"], mapDoubles["weight"], intVector);
    // GET THE MATERIALS


    // GET BEAM FILTERS AND ATTENUATORS
    sectionContents.clear();
    sectionContents = iniFile.readSection("attenuators", false);
    mapDoubles.clear();
    doubleVector.clear();
    stringVector.clear();
    this->beamFilters.clear();
    this->attenuators.clear();
    this->sample.clear();
    for (std::map<std::string, std::string>::const_iterator c_it = sectionContents.begin();
        c_it != sectionContents.end(); ++c_it)
    {
        // std::cout << c_it->first << " " << c_it->second << std::endl;
        content = c_it->second;
        iniFile.parseStringAsMultipleValues(content, doubleVector, -1.0);
        iniFile.parseStringAsMultipleValues(content, stringVector, std::string());
        if (doubleVector.size() == 0.0)
        {
            std::cout << "WARNING: Empty line in attenuators section. Offending key is: "<< std::endl;
            std::cout << "<" << c_it->first << ">" << std::endl;
            continue;
        }
        if (doubleVector[0] > 0.0)
        {
            if (c_it->first.substr(0, 9) == "BeamFilter")
            {
                //BeamFilter0 = 0, -, 0.0, 0.0, 1.0
                this->beamFilters.push_back(Layer(stringVector[1], \
                                                  doubleVector[2], doubleVector[3], doubleVector[4]));
            }
            else
            {
                // atmosphere = 0, -, 0.0, 0.0, 1.0
                // Matrix = 0, MULTILAYER, 0.0, 0.0, 45.0, 45.0, 0, 90.0
                if (stringVector.size() == 8 )
                {
                    // Matrix
                    this->setGeometry(doubleVector[5], doubleVector[6], doubleVector[7]);
                    if (stringVector[1] == "MULTILAYER")
                    {
                        std::cout << "MULTILAYER NOT PARSED YET" << std::endl;
                    }
                    else
                    {
                        this->sample.push_back(Layer(stringVector[1], \
                                                     doubleVector[2], doubleVector[3], doubleVector[4]));
                    }
                }
                else
                {
                    if (stringVector[1] == "Detector")
                    {
                        // DETECTOR
                        std::cout << "DETECTOR " << std::endl;
                        ;
                    }
                    else
                    {
                        // Attenuator
                        std::cout << "ATTENUATOR " << std::endl;
                        this->attenuators.push_back(Layer(stringVector[1], \
                                                doubleVector[2], doubleVector[3], doubleVector[4]));
                    }
                }
            }
        }
    }
}

void XRFConfig::setBeam(const std::vector<double> & energy, \
                        const std::vector<double> & weight, \
                        const std::vector<int> & characteristic, \
                        const std::vector<double> & divergency)
{
    this->beam.setBeam(energy, weight, characteristic, divergency);
}
