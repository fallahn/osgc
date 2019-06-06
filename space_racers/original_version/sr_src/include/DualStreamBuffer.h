/***dual stream buffer class.***
This class inherits from stl stream buffer and can be substituted
for any other buffer class. It takes 2 buffers as destinations on
construction which will both be written to by any stream using this
class as a buffer. For instance you can pass a buffer to a file
stream and a cout stream and then pass this instance to cout. Any
information then passed to cout will then be sent to both cout and
the file streams.

Example usage:

    std::streambuf* cout_sbuf = std::cout.rdbuf(); //hold copy of original buffer
    std::ofstream   fout("file.txt"); //create a file output stream
    dualbuf         dualout(std::cout.rdbuf(), fout.rdbuf()); //create an instance of this class using the buffers of cout and file output
    std::cout.rdbuf(&dualout); //assign the instance of this class as cout buffer (could be any other output stream)
    ***cout stuff in your program here**
    std::cout.rdbuf(cout_sbuf); //return original buffer to cout when done
*/

#ifndef DUALSTREAM
#define DUALSTREAM

#include <streambuf>
#include <sstream>
#include <iostream>
#include <ostream>

class dualbuf : public std::streambuf 
{
public:
	typedef std::char_traits<char> traits_type;
	typedef traits_type::int_type  int_type;

	dualbuf(std::streambuf* sb1, std::streambuf* sb2) :
		m_sb1(sb1), m_sb2(sb2) {}
 
private:
	std::streambuf *m_sb1, *m_sb2;

	int_type overflow(int_type c)
	{
		if(!traits_type::eq_int_type(c, traits_type::eof()))
		{
			c = m_sb1->sputc(c);
			if (!traits_type::eq_int_type(c, traits_type::eof()))
				c = m_sb2->sputc(c);
			return c;
		}
		else return traits_type::not_eof(c);
	}

    int sync() 
	{
		int rc = m_sb1->pubsync();
		if (rc != -1)
		rc = m_sb2->pubsync();
		return rc;
    }
};

//wrap the custom stream output in a class
//so destruction properly tidies up when program quits
class Logger
{
public:
	//we only need this once so let's make it a singleton
	static void Create() {static Logger l;};

private:
	Logger()
		: m_oldCout(std::cout.rdbuf()),
		m_logFile(std::ofstream("output.log", std::ios_base::app)),
		m_dualBuf(std::cout.rdbuf(), m_logFile.rdbuf())
	{
		//opened the file for logging so add a date to say where we started
		time_t timeNow = time(0);
		tm* localTime = localtime(&timeNow);
		char buffer[40];
		strftime(buffer, 40, "Log started on: %x %X", localTime);
		m_logFile << std::endl << buffer << std::endl;

		//update cout with custom dual buffer
		std::cout.rdbuf(&m_dualBuf);
	};

	~Logger()
	{
		//revert settings
		std::cout.rdbuf(m_oldCout);
		m_logFile.close();
	};

	std::streambuf* m_oldCout;
	std::ofstream m_logFile;
	dualbuf m_dualBuf;
};


#endif