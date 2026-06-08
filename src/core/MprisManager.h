#ifndef MPRIS_MANAGER_H
#define MPRIS_MANAGER_H

#include <QObject>
#include "MprisRootAdaptor.h"
#include "MprisPlayerAdaptor.h"

class MprisManager : public QObject {
    Q_OBJECT
public:
    explicit MprisManager(AudioEngine* audio, QObject* parent = nullptr);
    QString currentTitle() const;
    QString currentArtist() const;

public slots:
    void updateMetadata(const QVariantMap& track) { m_player->updateMetadata(track); }

signals:
    void nextRequested();
    void previousRequested();
    void metadataChanged();

private:
    MprisRootAdaptor* m_root;
    MprisPlayerAdaptor* m_player;
};

#endif
