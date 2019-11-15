
#include <glad/glad.h>
#include <string>
#include <iostream>

#ifndef XY_GL_CHECK_HPP_
#define XY_GL_CHECK_HPP_

#ifdef XY_DEBUG
#define glCheck(x) do{x; xy::Detail::glErrorCheck(__FILE__, __LINE__, #x);}while (false)
#else
#define glCheck(x) (x)
#endif //XY_DEBUG

namespace xy
{
    namespace Detail
    {
        static inline void glErrorCheck(const char* file, unsigned int line, const char* expression)
        {
            //get the last error
            GLenum errorCode = glGetError();
            if (errorCode != GL_NO_ERROR)
            {
                std::string fileString = file;
                std::string error = "Unknown error";
                std::string description = "No description";

                //decode the error code
                switch (errorCode)
                {
                case GL_INVALID_ENUM:
                {
                    error = "GL_INVALID_ENUM";
                    description = "An unacceptable value has been specified for an enumerated argument.";
                    break;
                }

                case GL_INVALID_VALUE:
                {
                    error = "GL_INVALID_VALUE";
                    description = "A numeric argument is out of range.";
                    break;
                }

                case GL_INVALID_OPERATION:
                {
                    error = "GL_INVALID_OPERATION";
                    description = "The specified operation is not allowed in the current state.";
                    break;
                }

                /*case GL_STACK_OVERFLOW:
                {
                    error = "GL_STACK_OVERFLOW";
                    description = "This command would cause a stack overflow.";
                    break;
                }

                case GL_STACK_UNDERFLOW:
                {
                    error = "GL_STACK_UNDERFLOW";
                    description = "This command would cause a stack underflow.";
                    break;
                }*/

                case GL_OUT_OF_MEMORY:
                {
                    error = "GL_OUT_OF_MEMORY";
                    description = "There is not enough memory left to execute the command.";
                    break;
                }

                }

                //log the error
                std::cout << "An internal OpenGL call failed in "
                    << fileString.substr(fileString.find_last_of("\\/") + 1) << "(" << line << ")."
                    << "\nExpression:\n   " << expression
                    << "\nError description:\n   " << error << "\n   " << description << "\n"
                    << std::endl;

                errorCode = glGetError(); //call until all errors are printed
            }
        }
    }
}

#endif //XY_GL_CHECK_HPP_
