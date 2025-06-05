/*

   \file                         Exceptions.h
   \brief                        Handles with exceptions
   \author                       Nicolas PIERRE
   \version                      1.0
   \date                         10/06/14
   Support :                     mail to : nicolas.pierre@cern.ch

 */

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__
#include <exception>
#include <string>

/*!
 * \class Exception
 * \brief Exception handling class, inheriting from std::exception
 */
class Exception : public std::exception
{
  private:
    std::string fStrError; /*!< Error String */

  public:
    /*!
     * \brief Constructor of Exception class
     * \param pStrError : Error message
     */
    Exception(std::string&& pStrError) { fStrError = pStrError; }
    /*!
     * \brief Destructor of Exception class
     */
    ~Exception() throw() {}
    /*!
     * \brief What to throw
     */
    const char* what() const throw();
};

#endif
