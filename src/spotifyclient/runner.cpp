#include "spotifyclient/runner.hpp"
#include "mainwindow.hpp"

std::vector<lib::log_message> SpotifyClient::Runner::log;

SpotifyClient::Runner::Runner(const lib::settings &settings,
	const lib::paths &paths, QWidget *parent)
	: QObject(parent),
	parentWidget(parent),
	settings(settings),
	paths(paths)
{
	path = QString::fromStdString(settings.spotify.path);
	process = new QProcess(parent);
	clientType = SpotifyClient::Helper::clientType(path);
}

SpotifyClient::Runner::~Runner()
{
	if (process != nullptr)
	{
		process->terminate();
		process->waitForFinished();
	}
}

void SpotifyClient::Runner::start()
{
	// Don't start if already running
	if (!settings.spotify.always_start && isRunning())
	{
		emit statusChanged({});
		return;
	}

	// Check if empty
	if (clientType == lib::client_type::none)
	{
		emit statusChanged(QStringLiteral("Client path is empty or invalid"));
		return;
	}

	// Check if path exists
	QFileInfo info(path);
	if (!info.exists())
	{
		emit statusChanged(QStringLiteral("Client path does not exist"));
		return;
	}

	// If using global config, just start
	if (settings.spotify.global_config && clientType == lib::client_type::spotifyd)
	{
		process->start(path, QStringList({QStringLiteral("--no-daemon")}));
		emit statusChanged({});
		return;
	}

	// Check if not logged in
	if (!isLoggedIn())
	{
		if (!Helper::getOAuthSupport(path))
		{
			emit statusChanged(QStringLiteral("Client unsupported, please upgrade and try again"));
			return;
		}

		login();
		return;
	}

	// Common arguments
	QStringList arguments({
		"--bitrate", QString::number(static_cast<int>(settings.spotify.bitrate)),
	});

	const auto initialVolume = QString::number(settings.spotify.volume);

	// librespot specific
	if (clientType == lib::client_type::librespot)
	{
		arguments.append({
			"--name", QString("%1 (librespot)").arg(APP_NAME),
			"--initial-volume", initialVolume,
			"--cache", QString::fromStdString(getCachePath().string()),
			"--autoplay", "on",
		});
	}
	else if (clientType == lib::client_type::spotifyd)
	{
		arguments.append({
			"--no-daemon",
			"--initial-volume", initialVolume,
			"--device-name", QString("%1 (spotifyd)").arg(APP_NAME),
		});
	}

	auto backend = QString::fromStdString(settings.spotify.backend);
	if (!backend.isEmpty())
	{
		arguments.append({
			"--backend", backend
		});
	}

	if (clientType == lib::client_type::librespot && settings.spotify.disable_discovery)
	{
		arguments.append("--disable-discovery");
	}

	const auto deviceType = settings.spotify.device_type;
	if (deviceType != lib::device_type::unknown)
	{
		const auto deviceTypeString = lib::enums<lib::device_type>::to_string(deviceType);
		if (!deviceTypeString.empty())
		{
			arguments.append({
				QStringLiteral("--device-type"),
				QString::fromStdString(deviceTypeString).remove(' '),
			});
		}
	}

	auto additional_arguments = QString::fromStdString(settings.spotify.additional_arguments);
	if (!additional_arguments.isEmpty())
	{
		arguments.append(additional_arguments.split(' '));
	}

	QProcess::connect(process, &QProcess::readyReadStandardOutput,
		this, &Runner::onReadyReadOutput);

	QProcess::connect(process, &QProcess::readyReadStandardError,
		this, &Runner::onReadyReadError);

	QProcess::connect(process, &QProcess::started,
		this, &Runner::onStarted);

	QProcess::connect(process, &QProcess::errorOccurred,
		this, &Runner::onErrorOccurred);

	lib::log::debug("starting: {} {}", path.toStdString(),
		joinArgs(arguments).toStdString());

	process->start(path, arguments);
}

void SpotifyClient::Runner::login()
{
	if (loginHelper != nullptr)
	{
		loginHelper->deleteLater();
		loginHelper = nullptr;
	}

	loginHelper = new Login(parentWidget);

	connect(loginHelper, &Login::loginSuccess,
		this, &Runner::onLoginSuccess);

	connect(loginHelper, &Login::loginFailed,
		this, &Runner::onLoginFailed);

	const auto cachePath = QString::fromStdString(getCachePath().string());
	loginHelper->run(path, cachePath);
}

auto SpotifyClient::Runner::isRunning() const -> bool
{
	return process == nullptr
		? SpotifyClient::Helper::running(path)
		: process->isOpen();
}

void SpotifyClient::Runner::logOutput(const QByteArray &output, lib::log_type logType)
{
	for (auto &line: QString(output).split('\n'))
	{
		if (line.isEmpty())
		{
			continue;
		}

		log.emplace_back(lib::date_time::now(), logType, line.toStdString());

		if (line.contains(QStringLiteral("Bad credentials")))
		{
			emit statusChanged(QStringLiteral("Bad credentials, please try again"));

			if (resetCredentials())
			{
				lib::log::debug("Credentials reset");
			}
		}
	}
}

auto SpotifyClient::Runner::joinArgs(const QStringList &args) -> QString
{
	QString result;
	for (auto i = 0; i < args.size(); i++)
	{
		const auto &arg = args.at(i);
		result.append(QString("%1%2%1%3")
			.arg(arg.contains(' ') ? "\"" : "", arg,
				i < args.size() - 1 ? " " : ""));
	}
	return result;
}

auto SpotifyClient::Runner::getCachePath() const -> ghc::filesystem::path
{
	return paths.cache() / "librespot";
}

auto SpotifyClient::Runner::isLoggedIn() const -> bool
{
	const auto path = getCachePath() / "credentials.json";
	return ghc::filesystem::exists(path);
}

auto SpotifyClient::Runner::resetCredentials() const -> bool
{
	const auto path = getCachePath() / "credentials.json";
	return ghc::filesystem::remove(path);
}

void SpotifyClient::Runner::onReadyReadOutput()
{
	logOutput(process->readAllStandardOutput(), lib::log_type::information);
}

void SpotifyClient::Runner::onReadyReadError()
{
	logOutput(process->readAllStandardError(), lib::log_type::error);
}

void SpotifyClient::Runner::onStarted()
{
	emit statusChanged({});
}

void SpotifyClient::Runner::onErrorOccurred(QProcess::ProcessError error)
{
	const auto message = Helper::processErrorToString(error);
	emit statusChanged(message);
}

void SpotifyClient::Runner::onLoginSuccess()
{
	if (!isLoggedIn())
	{
		lib::log::warn("Login successful, but not login found");
		emit statusChanged(QStringLiteral("Unknown error"));
		return;
	}

	loginHelper->deleteLater();
	loginHelper = nullptr;

	start();
}

void SpotifyClient::Runner::onLoginFailed(const QString &message)
{
	lib::log::warn(message.toStdString());
	emit statusChanged(message);

	loginHelper->deleteLater();
	loginHelper = nullptr;
}

auto SpotifyClient::Runner::getLog() -> const std::vector<lib::log_message> &
{
	return log;
}
