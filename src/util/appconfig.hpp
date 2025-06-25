#pragma once

#include <QStyle>

class AppConfig
{
public:
	static auto useNativeMenuBar() -> bool;

	static auto useClickableSlider() -> bool;

	static QString defaultStyleName;

private:
	AppConfig() = default;
};
