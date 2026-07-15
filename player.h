#ifndef PLAYER_H
#define PLAYER_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QListWidget>
#include <QKeyEvent>
#include <QShortcut>
#include <QAction>

enum class PlayMode {
    Sequential,
    Loop,
    SingleLoop,
    Random
};

struct PlayHistory {
    QString filePath;
    QString fileName;
    QDateTime playTime;
    qint64 duration;
    qint64 lastPosition;
};

inline QDataStream &operator<<(QDataStream &out, const PlayHistory &history) {
    out << history.filePath << history.fileName << history.playTime
        << history.duration << history.lastPosition;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, PlayHistory &history) {
    in >> history.filePath >> history.fileName >> history.playTime
       >> history.duration >> history.lastPosition;
    return in;
}

QT_BEGIN_NAMESPACE
namespace Ui { class Player; }
QT_END_NAMESPACE

class Player : public QMainWindow
{
    Q_OBJECT

public:
    Player(QWidget *parent = nullptr);
    ~Player();

private slots:
    void on_playButton_clicked();
    void on_stopButton_clicked();
    void on_forwardButton_clicked();
    void on_backwardButton_clicked();
    void on_previousButton_clicked();
    void on_nextButton_clicked();
    void on_progressSlider_valueChanged(int value);
    void on_volumeSlider_valueChanged(int value);
    void on_playModeButton_clicked();
    void on_playlistWidget_doubleClicked(const QModelIndex &index);
    void on_playlistWidget_customContextMenuRequested(const QPoint &pos);
    void on_volumeButton_clicked();

    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void updatePlayIcon();
    void handleMediaStatus(QMediaPlayer::MediaStatus status);
    void handleError(QMediaPlayer::Error error);
    void togglePlaylist();
    void openStreamUrl();
    void savePlaylist();
    void loadPlaylist();
    void clearPlaylist();
    void deletePlaylistItem();
    void showFileInfo();
    void sortPlaylist();
    void toggleMute();
    void playNext();
    void playPrevious();
    void openFile();
    void toggleFullScreen();
    void captureScreenshot();
    void saveSettings();
    void loadSettings();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    Ui::Player *ui;
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    QVideoWidget *videoWidget;
    QListWidget *playlistWidget;
    PlayMode playMode;
    bool isFullScreen;
    bool isPlaylistVisible;
    QMap<QString, qint64> lastPositions;
    QList<PlayHistory> playHistory;
    QShortcut *playShortcut;
    QShortcut *screenshotShortcut;
    QShortcut *fullscreenShortcut;
    QAction *openAction;
    QAction *exitAction;
    QAction *fullscreenAction;
    QAction *screenshotAction;

    void addToPlaylist(const QStringList &files);
    void addToHistory(const QString &filePath, qint64 duration, qint64 lastPosition);
    QString formatTime(qint64 milliseconds);
    QString getAppDataPath();
    QString getPlayModeText(PlayMode mode);
};

#endif // PLAYER_H