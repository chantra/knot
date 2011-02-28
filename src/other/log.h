/*!
 * \file log.h
 *
 * \author Marek Vavrusa <marek.vavrusa@nic.cz>
 *
 * \brief Logging facility.
 *
 * \note Loglevel defined in syslog.h, may be redefined in other backend, but
 * keep naming. LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG
 *
 * In standard mode, only LOG_ERR and LOG_WARNING is displayed and logged.
 * Verbose mode enables LOG_NOTICE and LOG_INFO for additional information.
 *
 * \addtogroup logging
 * @{
 */
#ifndef _KNOT_LOG_H_
#define _KNOT_LOG_H_

/*
 */
#include <syslog.h>

/* Logging facility setup. */

/*!
 * \brief Open log and stdio output for given masks.
 *
 * For bitmasks, refer to syslog.h
 *
 * \see syslog.h
 *
 * \retval  0 On success.
 * \retval <0 If an error occured.
 */
int log_open(int print_mask, int log_mask);

/*!
 * \brief Close log and stdio output.
 *
 * \retval  0 On success.
 * \retval <0 If an error occured.
 */
int log_close();

/*!
 * \brief Return positive number if open.
 *
 * \return 1 if open (boolean true)
 * \return 0 if closed (boolean false)
 */
int log_isopen();

/* Logging functions. */
int print_msg(int level, const char *msg, ...) __attribute__((format(printf, 2, 3)));

#define log_msg(level, msg...) \
	do { \
	if(log_isopen()) { \
		syslog((level), msg); \
	} \
	print_msg((level), msg); \
	} while (0)

/* Convenient logging. */
#define log_error(msg...)     log_msg(LOG_ERR, msg)
#define log_warning(msg...)   log_msg(LOG_WARNING, msg)
#define log_notice(msg...)    log_msg(LOG_NOTICE, msg)
#define log_info(msg...)      log_msg(LOG_INFO, msg)
#define log_debug(msg...)     log_msg(LOG_DEBUG, msg)

#endif /* _KNOT_LOG_H_ */

/*! @} */
