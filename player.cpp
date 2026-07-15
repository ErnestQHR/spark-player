#include "player.h"
#include "ui_player.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QScreen>
#include <QPixmap>
#include <QRandomGenerator>

Player::Player(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Player)
    , playMode(PlayMode::Sequential)
    , isFullScreen(false)
    , isPlaylistVisible(true)
{
    ui->setupUi(this);

    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    videoWidget = new QVideoWidget(this);

    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    ui->videoLayout->addWidget(videoWidget);

    playlistWidget = ui->playlistWidget;
    playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Player::updatePosition);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Player::updateDuration);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &Player::updatePlayIcon);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &Player::handleMediaStatus);
    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this, &Player::handleError);

    ui->progressSlider->setRange(0, 1000);
    ui->volumeSlider->setRange(0, 100);
    ui->volumeSlider->setValue(50);
    audioOutput->setVolume(0.5);

    playShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(playShortcut, &QShortcut::activated, this, &Player::on_playButton_clicked);

    screenshotShortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
    connect(screenshotShortcut, &QShortcut::activated, this, &Player::captureScreenshot);

    fullscreenShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(fullscreenShortcut, &QShortcut::activated, this, [this]() {
        if (isFullScreen) toggleFullScreen();
    });

    loadSettings();
    loadPlaylist();

    QMenu *fileMenu = menuBar()->addMenu("文件");
    openAction = fileMenu->addAction("打开文件", this, &Player::openFile, QKeySequence("Ctrl+O"));
    fileMenu->addSeparator();
    exitAction = fileMenu->addAction("退出", this, &Player::close);

    QMenu *playMenu = menuBar()->addMenu("播放");
    fullscreenAction = playMenu->addAction("全屏", this, &Player::toggleFullScreen);
    screenshotAction = playMenu->addAction("截图", this, &Player::captureScreenshot);

    menuBar()->addMenu("帮助");
}

Player::~Player()
{
    delete ui;
}

void Player::on_playButton_clicked()
{
    switch (mediaPlayer->playbackState()) {
    case QMediaPlayer::PlayingState:
        mediaPlayer->pause();
        break;
    case QMediaPlayer::PausedState:
        mediaPlayer->play();
        break;
    case QMediaPlayer::StoppedState:
        if (playlistWidget->count() > 0) {
            int currentRow = playlistWidget->currentRow();
            if (currentRow < 0) currentRow = 0;
            QString filePath = playlistWidget->item(currentRow)->data(Qt::UserRole).toString();
            mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
            mediaPlayer->play();
        } else {
            openFile();
        }
        break;
    }
}

void Player::on_stopButton_clicked()
{
    mediaPlayer->stop();
}

void Player::on_forwardButton_clicked()
{
    mediaPlayer->setPosition(mediaPlayer->position() + 10000);
}

void Player::on_backwardButton_clicked()
{
    mediaPlayer->setPosition(qMax(0LL, mediaPlayer->position() - 10000));
}

void Player::on_previousButton_clicked()
{
    playPrevious();
}

void Player::on_nextButton_clicked()
{
    playNext();
}

void Player::on_progressSlider_valueChanged(int value)
{
    if (mediaPlayer->duration() > 0) {
        qint64 position = (qint64)(value * mediaPlayer->duration()) / 1000;
        mediaPlayer->setPosition(position);
    }
}

void Player::on_volumeSlider_valueChanged(int value)
{
    audioOutput->setVolume(value / 100.0);
}

void Player::on_playModeButton_clicked()
{
    int currentMode = static_cast<int>(playMode);
    currentMode = (currentMode + 1) % 4;
    playMode = static_cast<PlayMode>(currentMode);
    ui->playModeButton->setText(getPlayModeText(playMode));
}



void Player::on_playlistWidget_doubleClicked(const QModelIndex &index)
{
    QString filePath = playlistWidget->item(index.row())->data(Qt::UserRole).toString();
    mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
    mediaPlayer->play();
}

void Player::on_playlistWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    QAction *playAction = menu.addAction("播放");
    QAction *deleteAction = menu.addAction("删除");
    QAction *saveAction = menu.addAction("保存列表");
    QAction *loadAction = menu.addAction("加载列表");
    QAction *clearAction = menu.addAction("清空列表");
    QAction *infoAction = menu.addAction("文件信息");
    QAction *sortAction = menu.addAction("排序");

    QAction *selectedAction = menu.exec(playlistWidget->mapToGlobal(pos));

    if (selectedAction == playAction) {
        on_playlistWidget_doubleClicked(playlistWidget->currentIndex());
    } else if (selectedAction == deleteAction) {
        deletePlaylistItem();
    } else if (selectedAction == saveAction) {
        savePlaylist();
    } else if (selectedAction == loadAction) {
        loadPlaylist();
    } else if (selectedAction == clearAction) {
        clearPlaylist();
    } else if (selectedAction == infoAction) {
        showFileInfo();
    } else if (selectedAction == sortAction) {
        sortPlaylist();
    }
}

void Player::on_volumeButton_clicked()
{
    toggleMute();
}



void Player::updatePosition(qint64 position)
{
    if (mediaPlayer->duration() > 0) {
        int value = static_cast<int>((position * 1000) / mediaPlayer->duration());
        ui->progressSlider->setValue(value);
    }
    ui->currentTimeLabel->setText(formatTime(position));
}

void Player::updateDuration(qint64 duration)
{
    ui->totalTimeLabel->setText(formatTime(duration));
}

void Player::updatePlayIcon()
{
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        ui->playButton->setText("暂停");
    } else {
        ui->playButton->setText("播放");
    }
}

void Player::handleMediaStatus(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        switch (playMode) {
        case PlayMode::Sequential:
            if (playlistWidget->currentRow() < playlistWidget->count() - 1) {
                playNext();
            } else {
                mediaPlayer->stop();
            }
            break;
        case PlayMode::Loop:
            if (playlistWidget->currentRow() < playlistWidget->count() - 1) {
                playNext();
            } else {
                playlistWidget->setCurrentRow(0);
                QString filePath = playlistWidget->item(0)->data(Qt::UserRole).toString();
                mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
                mediaPlayer->play();
            }
            break;
        case PlayMode::SingleLoop:
            mediaPlayer->setPosition(0);
            mediaPlayer->play();
            break;
        case PlayMode::Random:
            int randomRow = QRandomGenerator::global()->bounded(playlistWidget->count());
            playlistWidget->setCurrentRow(randomRow);
            QString filePath = playlistWidget->item(randomRow)->data(Qt::UserRole).toString();
            mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
            mediaPlayer->play();
            break;
        }
    }
}

void Player::handleError(QMediaPlayer::Error error)
{
    QMessageBox::warning(this, "播放错误", mediaPlayer->errorString());
}

void Player::togglePlaylist()
{
    isPlaylistVisible = !isPlaylistVisible;
    ui->playlistDock->setVisible(isPlaylistVisible);
}

void Player::openStreamUrl()
{
    bool ok;
    QString url = QInputDialog::getText(this, "打开流媒体", "请输入流媒体地址:", QLineEdit::Normal, "", &ok);
    if (ok && !url.isEmpty()) {
        mediaPlayer->setSource(QUrl(url));
        mediaPlayer->play();
    }
}

void Player::savePlaylist()
{
    QString filePath = QFileDialog::getSaveFileName(this, "保存播放列表", "", "M3U 播放列表 (*.m3u)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "#EXTM3U\n";
            for (int i = 0; i < playlistWidget->count(); ++i) {
                QString path = playlistWidget->item(i)->data(Qt::UserRole).toString();
                out << "#EXTINF:-1," << playlistWidget->item(i)->text() << "\n";
                out << path << "\n";
            }
            file.close();
        }
    }
}

void Player::loadPlaylist()
{
    QString filePath = QFileDialog::getOpenFileName(this, "加载播放列表", "", "M3U 播放列表 (*.m3u)");
    if (!filePath.isEmpty()) {
        clearPlaylist();
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QStringList files;
            QString line;
            while (!in.atEnd()) {
                line = in.readLine().trimmed();
                if (!line.isEmpty() && !line.startsWith("#")) {
                    files.append(line);
                }
            }
            file.close();
            addToPlaylist(files);
        }
    }
}

void Player::clearPlaylist()
{
    playlistWidget->clear();
}

void Player::deletePlaylistItem()
{
    int row = playlistWidget->currentRow();
    if (row >= 0) {
        delete playlistWidget->takeItem(row);
    }
}

void Player::showFileInfo()
{
    int row = playlistWidget->currentRow();
    if (row >= 0) {
        QString filePath = playlistWidget->item(row)->data(Qt::UserRole).toString();
        QFileInfo fileInfo(filePath);
        QMessageBox::information(this, "文件信息",
            QString("文件名: %1\n路径: %2\n大小: %3 KB\n修改时间: %4")
            .arg(fileInfo.fileName())
            .arg(fileInfo.absoluteFilePath())
            .arg(fileInfo.size() / 1024)
            .arg(fileInfo.lastModified().toString()));
    }
}

void Player::sortPlaylist()
{
    QStringList items;
    QStringList paths;
    for (int i = 0; i < playlistWidget->count(); ++i) {
        items.append(playlistWidget->item(i)->text());
        paths.append(playlistWidget->item(i)->data(Qt::UserRole).toString());
    }
    clearPlaylist();
    QMap<QString, QString> sortedMap;
    for (int i = 0; i < items.size(); ++i) {
        sortedMap[items[i]] = paths[i];
    }
    for (const QString &key : sortedMap.keys()) {
        QListWidgetItem *item = new QListWidgetItem(key);
        item->setData(Qt::UserRole, sortedMap[key]);
        playlistWidget->addItem(item);
    }
}

void Player::toggleMute()
{
    audioOutput->setMuted(!audioOutput->isMuted());
    ui->volumeButton->setText(audioOutput->isMuted() ? "取消静音" : "静音");
}

void Player::playNext()
{
    if (playlistWidget->count() > 0) {
        int currentRow = playlistWidget->currentRow();
        int nextRow = (currentRow + 1) % playlistWidget->count();
        playlistWidget->setCurrentRow(nextRow);
        QString filePath = playlistWidget->item(nextRow)->data(Qt::UserRole).toString();
        mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        mediaPlayer->play();
    }
}

void Player::playPrevious()
{
    if (playlistWidget->count() > 0) {
        int currentRow = playlistWidget->currentRow();
        int prevRow = (currentRow - 1 + playlistWidget->count()) % playlistWidget->count();
        playlistWidget->setCurrentRow(prevRow);
        QString filePath = playlistWidget->item(prevRow)->data(Qt::UserRole).toString();
        mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        mediaPlayer->play();
    }
}

void Player::openFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择媒体文件", "",
        "媒体文件 (*.mp4 *.avi *.mkv *.mov *.wmv *.mp3 *.wav *.flac *.ogg *.m3u8);;所有文件 (*)");
    if (!files.isEmpty()) {
        addToPlaylist(files);
        playlistWidget->setCurrentRow(playlistWidget->count() - files.size());
        QString filePath = playlistWidget->currentItem()->data(Qt::UserRole).toString();
        mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        mediaPlayer->play();
    }
}

void Player::toggleFullScreen()
{
    isFullScreen = !isFullScreen;
    if (isFullScreen) {
        showFullScreen();
        menuBar()->hide();
        ui->controlBar->hide();
        ui->playlistDock->hide();
    } else {
        showNormal();
        menuBar()->show();
        ui->controlBar->show();
        if (isPlaylistVisible) {
            ui->playlistDock->show();
        }
    }
}

void Player::captureScreenshot()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap screenshot = screen->grabWindow(videoWidget->winId());
    QString fileName = QString("screenshot_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QDir::homePath() + "/Pictures/" + fileName;
    screenshot.save(filePath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::homePath() + "/Pictures"));
}

void Player::saveSettings()
{
    QSettings settings("spark_player", "spark_player");
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("volume", audioOutput->volume());
    settings.setValue("playMode", static_cast<int>(playMode));
    settings.setValue("lastFile", playlistWidget->currentRow() >= 0 ?
        playlistWidget->item(playlistWidget->currentRow())->data(Qt::UserRole).toString() : "");

    QString playlistPath = getAppDataPath() + "/default.m3u";
    QFile file(playlistPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "#EXTM3U\n";
        for (int i = 0; i < playlistWidget->count(); ++i) {
            QString path = playlistWidget->item(i)->data(Qt::UserRole).toString();
            out << "#EXTINF:-1," << playlistWidget->item(i)->text() << "\n";
            out << path << "\n";
        }
        file.close();
    }

}

void Player::loadSettings()
{
    QSettings settings("spark_player", "spark_player");
    restoreGeometry(settings.value("windowGeometry").toByteArray());
    audioOutput->setVolume(settings.value("volume", 0.5).toDouble());
    ui->volumeSlider->setValue(static_cast<int>(audioOutput->volume() * 100));
    playMode = static_cast<PlayMode>(settings.value("playMode", 0).toInt());
    ui->playModeButton->setText(getPlayModeText(playMode));
}

void Player::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        on_backwardButton_clicked();
        break;
    case Qt::Key_Right:
        on_forwardButton_clicked();
        break;
    case Qt::Key_Up:
        ui->volumeSlider->setValue(qMin(100, ui->volumeSlider->value() + 5));
        break;
    case Qt::Key_Down:
        ui->volumeSlider->setValue(qMax(0, ui->volumeSlider->value() - 5));
        break;
    case Qt::Key_M:
        toggleMute();
        break;
    case Qt::Key_Delete:
        deletePlaylistItem();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void Player::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void Player::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (videoWidget->underMouse()) {
        toggleFullScreen();
    }
    QMainWindow::mouseDoubleClickEvent(event);
}

void Player::addToPlaylist(const QStringList &files)
{
    for (const QString &filePath : files) {
        QFileInfo fileInfo(filePath);
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setData(Qt::UserRole, filePath);
        playlistWidget->addItem(item);
    }
}

QString Player::formatTime(qint64 milliseconds)
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;

    if (hours > 0) {
        return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0'))
                                   .arg(minutes, 2, 10, QChar('0'))
                                   .arg(seconds, 2, 10, QChar('0'));
    }
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0'));
}

QString Player::getAppDataPath()
{
#ifdef Q_OS_MACOS
    return QDir::homePath() + "/Library/Application Support/spark_player";
#elif defined(Q_OS_LINUX)
    return QDir::homePath() + "/.local/share/spark_player";
#elif defined(Q_OS_WIN)
    return QString::fromUtf8(qgetenv("APPDATA")) + "/spark_player";
#else
    return QDir::homePath() + "/.spark_player";
#endif
}

QString Player::getPlayModeText(PlayMode mode)
{
    switch (mode) {
    case PlayMode::Sequential:
        return "顺序";
    case PlayMode::Loop:
        return "循环";
    case PlayMode::SingleLoop:
        return "单曲";
    case PlayMode::Random:
        return "随机";
    default:
        return "顺序";
    }
}