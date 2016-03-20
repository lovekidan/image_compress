#ifndef IC_EXCEPTION_H
#define IC_EXCEPTION_H

#include <exception>
using namespace std;

#define DECLARE_EXCEPTION(className) \
class className: public exception \
{ \
public: \
    className(const char* msg): _exception_msg(msg){}\
    const char *what()const throw() {return _exception_msg;}\
private: \
	const char* _exception_msg; \
};

DECLARE_EXCEPTION(ICException)
DECLARE_EXCEPTION(InvalidImageException)
DECLARE_EXCEPTION(ImageSizeException)
DECLARE_EXCEPTION(StretchException)

#endif