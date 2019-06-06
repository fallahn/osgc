/** settings.h Torleif West 17/dec/2007 
 *
 * Loads settings from file, then map them for quick access. 
 * Allows comments in the settings file and trims wasted white space. 
 *
 */
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <map>      // map of properties
#include <string>   // for std::string
#include <fstream>   // opening file
#include <ostream>   // saving file

class BaseSettings
{
	/**
		Class stores strings in a map, with a value for each. 
		This allows quick access to the data
	*/
private:
	//ctor requires a pointer to custom function for creating default cfg file for new setting types
	typedef void (*fileCallback)(std::ifstream& ifs, std::string& filename);

    // Map of two strings
    std::map<std::string, std::string> m_data;

    // Use numbers char values as UNIX has different new line char
    static const char CHAR_LINE1 = 59;   // windows endline
    static const char CHAR_LINE2 = 10;   // UNIX endline
    static const char CHAR_SPACE = 32;   // ' ' char
    static const char CHAR_ENDFILE = -1;// EOF
    static const char CHAR_EQUALS = 61;   // = char
    static const char CHAR_COMMENT = 35;// # char
    static const char NUMBER_POINT = 46;// ASCII value for .
    static const char NUMBER_NINE = 57;   // ASCII value for 9
    static const char NUMBER_ZERO = 48;   // ASCII value for 0

    // file name of this settings file (used if saved)
    std::string m_settingsFile;

protected:         
    // --- Removes white space ---
    // \param targ string of to trim
    // \returns string with no white space at start or end
    std::string m_trim(const std::string &targ)
	{
		std::string retstr;
		int startStr = 0;
		int endStr = targ.size();

		if(targ.size() == 0) return targ;

		for(int i = 0; i < (int)targ.size(); i++)
		{
			char c = targ.at(i);
			if(c != CHAR_LINE1 && c != CHAR_LINE2 &&
				c != CHAR_ENDFILE && c != CHAR_SPACE )
			{
				startStr = i;
				break;
			}
		}
		for(int i = (int)targ.size()-1; i > 0; i--)
		{
			char c = targ.at(i);
			if(c != CHAR_LINE1 && c != CHAR_LINE2 && 
				c != CHAR_ENDFILE && c != CHAR_SPACE )
			{
				endStr = i;
				break;
			}
		}
		// rebuild string
		for(int i = startStr; i <= endStr; i++)
		{
			std::string tmpchar;
			tmpchar = targ.at(i);
			retstr.append(tmpchar);
		}
		return retstr;
    }

    // --- Returns value in map ---
    // \param key the string key stored in the map
    // \param noret return value if no string found
    // \returns string in map, or noret if not found
    std::string m_getValue(std::string key, std::string noret)
	{
        std::map<std::string, std::string>::iterator it;
        it = m_data.find(key);
        if(it != m_data.end())
		{
            return (*it).second;
        }
        return noret;
    }
            
    // --- Returns the size of the map---
    // \returns int how many items in data map
    int size()
	{
        return static_cast<int>(m_data.size());
    }
            
    // --- Converts string to double ---
    // \param s the string to convert
    // \returns double floating point number of string 
    double strToDob(const std::string &s)
	{
		double retvalue = 0.0;
		int digitPlacing = 0;   // place in str where you've read to
		int digitPoint = -1;   // place where the point is

		for(int i = 0; i < static_cast<int>(s.size()); i++)
		{
			// convert char to number for double
			if(s.at(i) >= NUMBER_ZERO && s.at(i) <= NUMBER_NINE)
			{
				if(digitPoint == -1)
				{   //adding 
					retvalue *= 10;
					retvalue += s.at(i) - NUMBER_ZERO;
				} 
				else
				{
					int add = 10;// find power of 10 to decimal place
					for(int m = 0; m < (digitPlacing-digitPoint); m++)
					add *= 10;
					retvalue+=static_cast<double>(s.at(i)-NUMBER_ZERO) / static_cast<double>(add);
				}
				digitPlacing++;
			}
			// running into point
			if(s.at(i) == NUMBER_POINT && digitPoint == -1)
			{
				digitPoint = digitPlacing;
			}
		}
        return retvalue;
    }


    // --- Saves a the value in file, and changes in map (slow atm) ---
    // \param saveStr the value in the map to change
    // \param newValue the value that will replace the old value 
    void m_setValue(std::string saveStr, std::string newValue)
	{
		// bail out on failure
		std::string oldValue = m_getValue(saveStr, "");
		std::string saveFile;

		// bail out if value dosn't exist
		if(oldValue.compare("") == 0) return;

		// save file in memory
		std::ifstream ifs(m_settingsFile.c_str(), std::ifstream::in);
		while (ifs.good())
		{
			char c = ifs.get();
			if(c != CHAR_ENDFILE)
			{
				std::string tmpchar;
				tmpchar = c;
				saveFile.append(tmpchar);
			}
		}
		ifs.close();

		// go through each character, and find the string your replacing
		bool swch = true;
		bool stopReading = false;
		char c;
		std::string mproperty = "";
		std::string value = "";
		for(int i = 0; i < static_cast<int>(saveFile.size()); i++)
		{
			c = saveFile.at(i);
			//printf("%d ", int(c)); // debug
			if(c == CHAR_EQUALS)
			{
				swch = false;
			}
			if(c == CHAR_COMMENT)
			{
				stopReading = true;
			}
			// ignore white space
			if(c != CHAR_EQUALS && !stopReading)
			{
				std::string tmpchar;
				tmpchar = c;
				if(swch)
				{
					mproperty.append(tmpchar);
				} 
				else
				{
					// found the value, save and bail.
					if(m_trim(mproperty).compare(saveStr)==0)
					{
						saveFile.replace(i + 1, oldValue.size(), newValue);

						// save the new value in the map
						m_data.erase (saveStr);
						m_data.insert(std::pair<std::string, std::string>(saveStr, newValue));
                  
						// and finally in the map
						std::ofstream savFil(m_settingsFile.c_str(), std::ofstream::trunc);
						savFil << saveFile;
						savFil.close();

						//Console::lout << "Wrote parameter " << saveStr << " with value " << newValue << std::endl;
						return;
					}
					value.append(tmpchar);
				}
			}
			// read to the end of the line
			if(c == CHAR_LINE1 || c == CHAR_LINE2 || c == CHAR_ENDFILE)
			{
				mproperty = "";
				value = "";
				swch = true;
				stopReading = false;
			}
		}
    }

public:
             
    // --- Loads the settings file ---
    // \param filename the string file name to load
    BaseSettings(std::string filename, fileCallback newfile) 
	{ 
		m_settingsFile = filename;

		std::ifstream ifs(filename.c_str(), std::ifstream::in);
		
		//create file if it doesn't exist
		if(ifs.fail()) newfile(ifs, filename);
		
		bool swch = true;
		bool stopReading = false;
		char c;
		std::string mproperty = "";
		std::string value = "";

		while(ifs.good())
		{
			c = (char) ifs.get();
			//printf("%d ", int(c)); // debug
			if(c == CHAR_EQUALS)
			{
				swch = false;
			}
			if(c == CHAR_COMMENT)
			{
				stopReading = true;
			}
			// ignore white space
			if(c != CHAR_EQUALS && !stopReading)
			{
				std::string tmpchar;
				tmpchar = c;
				if(swch)
				{
					mproperty.append(tmpchar);
				} 
				else
				{
					value.append(tmpchar);
				}
			}
			// read to the end of the line
			if(c == CHAR_LINE1 || c == CHAR_LINE2 || c == CHAR_ENDFILE)
			{
				if(!swch)
				{
					//printf("Inserting :%s=%s\n", 
					//   trim(mproperty).c_str(), trim(value).c_str());
					m_data.insert( std::pair<std::string, std::string>(m_trim(mproperty),m_trim(value)));
				}
				mproperty = "";
				value = "";
				swch = true;
				stopReading = false;
			}
		}
		ifs.close();
    }
            
    // --- Deconstructor ---
    virtual ~BaseSettings()
	{ 
		m_data.clear(); 
    }
};

class VideoSettings : public BaseSettings
{
public:
	VideoSettings(std::string filename) : BaseSettings(filename, &m_initNewFile){};

	//returns the stored resolution or empty if not found
	sf::VideoMode getResolution()
	{
		sf::VideoMode retVal(640u, 480u);
		std::string value = m_getValue("resolution", "missing");
		if(value != "missing")
		{
			sf::Uint32 res = atoi(value.c_str());
			if(res) //not 0
			{
				retVal.width = res >> 16;
				retVal.height = res & 0xffff;
			}
			else setResolution(retVal); //store a default value
		}
		return retVal;
	}

	//stores the resolution paramter
	void setResolution(const sf::VideoMode& resolution)
	{
		sf::Uint32 res = resolution.width << 16 | resolution.height;
		m_setValue("resolution", std::to_string(res));
	}

	//returns the number of aa samples, or 0 if not found
	sf::Uint8 getMultiSamples()
	{
		std::string value = m_getValue("samples", "missing");
		if(value != "missing")
		{
			return atoi(value.c_str());
		}
		else return 0;
	}

	//stores the number of aa samples
	void setMultiSamples(sf::Uint8 numSamples)
	{
		if(numSamples > 8) numSamples = 8;
		m_setValue("samples", std::to_string(numSamples));
	}

	//TODO all these bool settings could be one function

	//returns the full screen setting
	const bool getFullcreen()
	{
		std::string value = m_getValue("fullscreen", "missing");
		if(value != "missing") return (atoi(value.c_str()) != 0);
		else return false;
	}

	//stores full screen setting
	void setFullScreen(bool b)
	{
		if(b) m_setValue("fullscreen", "1");
		else m_setValue("fullscreen", "0");
	}

	//returns shader setting
	const bool getShaders()
	{
		std::string value = m_getValue("shaders", "missing");
		if(value != "missing") return (atoi(value.c_str()) != 0);
		else return false;
	}

	//stores the shader setting
	void setShaders(bool b)
	{
		if(b) m_setValue("shaders", "1");
		else m_setValue("shaders", "0");
	}

	//gets the vsync setting
	const bool getVsync()
	{
		std::string value = m_getValue("vsync", "missing");
		if(value != "missing") return (atoi(value.c_str()) != 0);
		else return false;
	}

	//stores the vsync setting
	void setVsync(bool b)
	{
		if(b) m_setValue("vsync", "1");
		else m_setValue("vsync", "0");
	}

	float getSfxVolume()
	{
		std::string value = m_getValue("sfx", "missing");
		if(value != "missing")
		{
			return static_cast<float>(atoi(value.c_str()));
		}
		else return 0.f;
	}

	void setSfxVolume(float value)
	{
		m_setValue("sfx", std::to_string(static_cast<int>(value)));
	}

	float getMusicVolume()
	{
		std::string value = m_getValue("music", "missing");
		if(value != "missing")
		{
			return static_cast<float>(atoi(value.c_str()));
		}
		else return 0.f;
	}

	void setMusicVolume(float volume)
	{
		m_setValue("music", std::to_string(static_cast<int>(volume)));
	}

private:
	//custom function for creating default settings when file not found
	static void m_initNewFile(std::ifstream& ifs, std::string& filename)
	{
		std::ofstream cfg(filename.c_str());
		cfg << "#Altering this by hand is not recommended." << std::endl;
		cfg << "#If the program fails to run after altering this file" << std::endl;
		cfg << "#delete the cfg from your hard drive and relaunch the program." << std::endl;
		cfg << "resolution = 0" << std::endl;
		cfg << "samples = 0" << std::endl;
		cfg << "fullscreen = 0" << std::endl;
		cfg << "shaders = 0" << std::endl;
		cfg << "vsync = 0" << std::endl;
		cfg << "sfx = 100" << std::endl;
		cfg << "music = 100" << std::endl;
		cfg.close();
		ifs.open(filename.c_str(), std::ifstream::in);
	}
};
#endif //SETTINGS_H_