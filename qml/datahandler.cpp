/*
 * Copyright (C) 2016  Aditya Dev Sharma <aditya.sharma156696@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "datahandler.h"

#include <QtCore/QMap>
#include <QtCore/QDebug>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

DataHandler::DataHandler(QObject* parent): QObject(parent) {

    connect(this, &DataHandler::addedToWallet, this, &DataHandler::setDBusConnections);
    connect(this, &DataHandler::showUserCredentialsPage, this,
            &DataHandler::onShowUserCredentialsPage);
    m_wallet = KWallet::Wallet::openWallet("KDENowWallet",
                                            0,
                                            KWallet::Wallet::Synchronous
                                          );
    if (!m_wallet->hasFolder("KDENow")) {
        emit showUserCredentialsPage();
    }
    else {
        emit addedToWallet();
    }
    KWallet::Wallet::closeWallet("KDENowWallet", true);
}

void DataHandler::onEventMapReceived()
{
    QDBusInterface* interface = new QDBusInterface("org.kde.kdenow", "/Event");
    QDBusReply<QVariantMap> reply = interface->call("getMap");
    if (reply.isValid()) {
        qDebug() << "Valid Reply received from org.kde.kdenow /Event getMap";
        qDebug() << reply.value();
    }
    else {
        qDebug() << "Did not receive a valid reply from org.kde.kdenow /Event getMap";
        return;
    }

    m_map = reply.value();
    emit eventDataReceived();
}

void DataHandler::onFlightMapReceived()
{
    QDBusInterface* interface = new QDBusInterface("org.kde.kdenow", "/Flight");
    QDBusReply<QVariantMap> reply = interface->call("getMap");
    if (reply.isValid()) {
        qDebug() << "Valid Reply received from org.kde.kdenow /Flight getMap";
        qDebug() << reply.value();
    }
    else {
        qDebug() << "Did not receive a valid reply from org.kde.kdenow /Flight getMap";
        return;
    }

    m_map = reply.value();
    emit flightDataReceived();
}

void DataHandler::onHotelMapReceived()
{
    QDBusInterface* interface = new QDBusInterface("org.kde.kdenow", "/Hotel");
    QDBusReply<QVariantMap> reply = interface->call("getMap");
    if (reply.isValid()) {
        qDebug() << "Valid Reply received from org.kde.kdenow /Hotel getMap";
        qDebug() << reply.value();
    }
    else {
        qDebug() << "Did not receive a valid reply from org.kde.kdenow /Hotel getMap";
        return;
    }

    m_map = reply.value();
    emit hotelDataReceived();
}

void DataHandler::onRestaurantMapReceived()
{
    QDBusInterface* interface = new QDBusInterface("org.kde.kdenow", "/Restaurant");
    QDBusReply<QVariantMap> reply = interface->call("getMap");
    if (reply.isValid()) {
        qDebug() << "Valid Reply received from org.kde.kdenow /Restaurant getMap";
        qDebug() << reply.value();
    }
    else {
        qDebug() << "Did not receive a valid reply from org.kde.kdenow /Restaurant getMap";
        return;
    }

    m_map = reply.value();
    emit restaurantDataReceived();
}

QVariantMap DataHandler::getMap()
{
    return m_map;
}

void DataHandler::onOkPressed(QString imapServer, QString imapPort, QString username,
                              QString password)
{
    m_wallet = KWallet::Wallet::openWallet("KDENowWallet",
                                            0,
                                            KWallet::Wallet::Synchronous
                                          );
    if (m_wallet->createFolder("KDENow") && m_wallet->setFolder("KDENow")) {
        qDebug() << "Created and set the folder in KWallet";
    }
    else {
        qDebug() << "Could not create or set the folder";
        return;
    }

    QMap<QString, QString> walletMap;
    walletMap.insert("imapServer", imapServer);
    walletMap.insert("imapPort", imapPort);
    walletMap.insert("username", username);
    walletMap.insert("password", password);

    m_view.close();
    int fail = m_wallet->writeMap("KDENowKey", walletMap);
    if (fail) {
        qDebug() << "Could not write map";
        return;
    }
    else {
        qDebug() << "Succesfully written map";
    }
    KWallet::Wallet::closeWallet("KDENowWallet", true);
    emit addedToWallet();
}

void DataHandler::setDBusConnections()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusInterface* interface = new QDBusInterface("org.kde.kdenow", "/KDENow");

    //Call a method, to start the kdenowd daemon if it hasn't yet started
    QDBusReply<QString> reply = interface->call("startDaemon");
    if (reply.isValid()) {
        qDebug() << "Valid Reply received from org.kde.kdenow /KDENow";
        qDebug() << reply.value();
    }
    else {
        qDebug() << "Did not receive a valid reply from org.kde.kdenow /KDENow";
        return;
    }

    dbus.connect("org.kde.kdenow", "/Event", "org.kde.kdenow.event",
                 "update", this, SLOT(onEventMapReceived()));
    dbus.connect("org.kde.kdenow", "/Flight", "org.kde.kdenow.flight",
                 "update", this, SLOT(onFlightMapReceived()));
    dbus.connect("org.kde.kdenow", "/Hotel", "org.kde.kdenow.hotel",
                 "update", this, SLOT(onHotelMapReceived()));
    dbus.connect("org.kde.kdenow", "/Restaurant", "org.kde.kdenow.restaurant",
                 "update", this, SLOT(onRestaurantMapReceived()));
}

void DataHandler::onShowUserCredentialsPage()
{
    m_view.setSource(QUrl("qrc:/UserCredentials.qml"));
    QObject* object = m_view.rootObject();
    connect(object, SIGNAL(okSignal(QString, QString, QString, QString)),
           this, SLOT(onOkPressed(QString, QString, QString, QString)));
    //connect(object, SIGNAL(cancelSignal()), this, SLOT(onCancelPressed()));
    m_view.show();
}
