#include "filedownloader.h"

FileDownloader::FileDownloader(int id, QUrl imageUrl, QObject *parent) : QObject(parent) {
    connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply *)), this, SLOT(fileDownloaded(QNetworkReply *)));
    myUrl = imageUrl;
    myId = id;
    //Refresh(); // Commented out to avoid downloading upon creation
}

void FileDownloader::Refresh() {
    QNetworkRequest request(myUrl);
    m_WebCtrl.get(request);
}

FileDownloader::~FileDownloader() { }

void FileDownloader::fileDownloaded(QNetworkReply *pReply) {
    if(pReply->error() != QNetworkReply::NoError) {
        qDebug() << "FileDownloader: Get Request Error!";
        emit downloaded(-1);    // Signal error
    } else {
        m_DownloadedData = pReply->readAll();
        emit downloaded(myId);
    }
    pReply->deleteLater();
}

void FileDownloader::newUrl(const QUrl &value) {
    myUrl = value;
    Refresh();
}

QByteArray FileDownloader::downloadedData() const {
    return m_DownloadedData;
}

QUrl FileDownloader::CurrentUrl() const {
    return myUrl;
}
