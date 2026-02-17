#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "app/Application.h"

#ifdef Q_OS_WIN
#include <atomic>
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#include <string>
#include <thread>
#endif

#ifdef Q_OS_WIN
class StdFdLineFilter final {
public:
    explicit StdFdLineFilter(int targetFd)
        : m_targetFd(targetFd) {}

    bool start() {
        if (m_started) {
            return true;
        }

        std::fflush(nullptr);

        int pipeFds[2] = {-1, -1};
        if (_pipe(pipeFds, 4096, _O_TEXT) != 0) {
            return false;
        }

        m_originalFd = _dup(m_targetFd);
        if (m_originalFd == -1) {
            _close(pipeFds[0]);
            _close(pipeFds[1]);
            return false;
        }

        if (_dup2(pipeFds[1], m_targetFd) == -1) {
            _close(pipeFds[0]);
            _close(pipeFds[1]);
            _close(m_originalFd);
            m_originalFd = -1;
            return false;
        }

        m_pipeRead = pipeFds[0];
        m_pipeWrite = pipeFds[1];
        m_started = true;

        m_thread = std::thread([this]() { processLoop(); });
        return true;
    }

    void stop() {
        if (!m_started) {
            return;
        }

        std::fflush(nullptr);

        if (m_originalFd != -1) {
            _dup2(m_originalFd, m_targetFd);
        }

        if (m_pipeWrite != -1) {
            _close(m_pipeWrite);
            m_pipeWrite = -1;
        }

        if (m_thread.joinable()) {
            m_thread.join();
        }

        if (m_pipeRead != -1) {
            _close(m_pipeRead);
            m_pipeRead = -1;
        }

        if (m_originalFd != -1) {
            _close(m_originalFd);
            m_originalFd = -1;
        }

        m_started = false;
    }

    ~StdFdLineFilter() {
        stop();
    }

private:
    static bool shouldDropLine(const std::string &line) {
        return line.find("cs.state() is") != std::string::npos &&
               line.find("connection_state_ is") != std::string::npos;
    }

    void processLoop() {
        constexpr int kBufferSize = 512;
        char buffer[kBufferSize];

        while (true) {
            const int bytesRead = _read(m_pipeRead, buffer, kBufferSize);
            if (bytesRead <= 0) {
                break;
            }

            m_pending.append(buffer, static_cast<size_t>(bytesRead));

            size_t start = 0;
            while (true) {
                const size_t end = m_pending.find('\n', start);
                if (end == std::string::npos) {
                    m_pending.erase(0, start);
                    break;
                }

                std::string line = m_pending.substr(start, end - start + 1);
                if (!shouldDropLine(line) && m_originalFd != -1) {
                    _write(m_originalFd, line.data(), static_cast<unsigned int>(line.size()));
                }
                start = end + 1;
            }
        }

        if (!m_pending.empty() && !shouldDropLine(m_pending) && m_originalFd != -1) {
            _write(m_originalFd, m_pending.data(), static_cast<unsigned int>(m_pending.size()));
        }
        m_pending.clear();
    }

    int m_targetFd{-1};
    int m_originalFd{-1};
    int m_pipeRead{-1};
    int m_pipeWrite{-1};
    std::thread m_thread;
    std::string m_pending;
    bool m_started{false};
};
#endif

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    StdFdLineFilter stdoutFilter(_fileno(stdout));
    StdFdLineFilter stderrFilter(_fileno(stderr));
    stdoutFilter.start();
    stderrFilter.start();
#endif

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QCoreApplication::setApplicationName(QStringLiteral("Voxter"));
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Voxter"));
    app.setQuitOnLastWindowClosed(true);

    Application application;
    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        &application,
        [&application]() { application.shutdown(); });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("app", &application);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] {
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.loadFromModule("Voxter", "Main");
    application.initialize();

    const int result = app.exec();

#ifdef Q_OS_WIN
    stdoutFilter.stop();
    stderrFilter.stop();
#endif

    return result;
}
