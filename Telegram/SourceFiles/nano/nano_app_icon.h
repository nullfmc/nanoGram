/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Nano {

enum class AppIcon {
	Classic,
	Glass,
	Blue,
	Green,
	Extera,
};

inline constexpr auto kAppIconsCount = 5;

[[nodiscard]] AppIcon CurrentAppIcon();
[[nodiscard]] const QImage &AppIconImage(AppIcon icon);
[[nodiscard]] const QImage &AppIconImageSmall(AppIcon icon);

// Saves the choice and repaints windows, taskbar and tray.
void SetAppIcon(AppIcon icon);

} // namespace Nano
