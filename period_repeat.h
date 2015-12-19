#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


/**
 * Repeat periodically an action at a timed interval.
 * Conveniece function to avoid repetitive code.
 * Uses boost::asio::deadline_timer. See Boos docs for details
 */
void period_repeat(
                   unsigned long interval_msec,      ///< how frequently to do it (msec)>.
                   std::function< void() > action,   ///<what to do>.
                   std::function< bool() > condition ///<repeat while this return true>.
                  )
{
    // Construct a timer with an absolute expiry time so that we can pace
    // without drift
    boost::asio::io_service     ios;
    boost::posix_time::milliseconds interval(interval_msec);
    boost::posix_time::ptime  next_t=boost::posix_time::microsec_clock::local_time()+interval;
    boost::asio::deadline_timer timer(ios,next_t);
    timer.wait();
    ios.run();

    while (true) {

        action();

        if (condition()) {
            next_t += interval;
            timer.expires_at(next_t);
            timer.wait();
        } else
            break;
    }
}