#ifndef QTAV_COMPAT_H
#define QTAV_COMPAT_H


//avutil: error.h
#ifndef av_err2str
#define AV_ERROR_MAX_STRING_SIZE 64
/**
 * Convenience macro, the return value should be used only directly in
 * function arguments but never stand-alone.
 */
#define av_err2str(errnum) \
	av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)

#endif //av_err2str

#endif
