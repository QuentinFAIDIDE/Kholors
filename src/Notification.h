#ifndef DEF_NOTIFICATION_HPP
#define DEF_NOTIFICATION_HPP

#include <string>

/**
 * @brief Various types of notifications.
 *
 */
enum NotificationKind
{
    INFO_NOTIF_TYPE,
    WARNING_NOTIF_TYPE,
    ERROR_NOTIF_TYPE,
};

/**
 * @brief Notification data model.
 *
 */
struct Notification
{
    int id;                          /**< A unique increasing identifier of the notification assigned when received. */
    std::string message;             /**< Message of the notification */
    unsigned int expirationTimeUnix; /**< Unix timestamp at which this notif should disapear */
    NotificationKind type;           /**< Notification type */
};

#endif // DEF_NOTIFICATION_HPP