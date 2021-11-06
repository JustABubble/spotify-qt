#pragma once

#ifdef USE_DBUS

#include "lib/log.hpp"

#include "enum/notificationaction.hpp"

#include <utility>

#include <QList>
#include <QTextDocument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QCoreApplication>

/**
 * Notifications using d-bus through the
 * org.freedesktop.Notifications interface
 */
class DbusNotifications: public QObject
{
Q_OBJECT

public:
	DbusNotifications(QObject *parent);

	auto getCapabilities() -> QList<QString>;

	void notify(const QString &title, const QString &message,
		const QString &imagePath, int timeout);

	void setOnAction(std::function<void(NotificationAction)> callback);

private:
	QDBusConnection dbus;
	uint notificationId = 0;

	std::function<void(NotificationAction)> onAction;

	auto isConnected() -> bool;

	void onActionInvoked(uint id, const QString &actionKey);
};

#endif