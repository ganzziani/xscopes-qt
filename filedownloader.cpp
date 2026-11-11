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
    if(pReply->error() == QNetworkReply::NoError) {
        m_DownloadedData = pReply->readAll();
    } else {
        m_DownloadedData.clear();
        qDebug() << "FileDownloader: Get Request Error:" << pReply->errorString();
    }
    emit downloaded(pReply->error(), pReply->errorString());
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
