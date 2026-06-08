#ifndef MPRIS_ROOT_ADAPTOR_H
#define MPRIS_ROOT_ADAPTOR_H

#include <QtDBus/QDBusAbstractAdaptor>
#include <QStringList>
#include <QCoreApplication>

class MprisRootAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ CanQuit)
    Q_PROPERTY(bool CanRaise READ CanRaise)
    Q_PROPERTY(bool HasTrackList READ HasTrackList)
    Q_PROPERTY(QString Identity READ Identity)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes)

public:
    explicit MprisRootAdaptor(QObject* parent) : QDBusAbstractAdaptor(parent) {}
    bool CanQuit() const { return true; }
    bool CanRaise() const { return true; }
    bool HasTrackList() const { return false; }
    QString Identity() const { return "morph"; }
    QString DesktopEntry() const { return "morph"; }
    QStringList SupportedUriSchemes() const { return {}; }
    QStringList SupportedMimeTypes() const { return {}; }

public slots:
    void Raise() {}
    void Quit() { qApp->quit(); }
};

#endif
