/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "nano/nano_app_icon.h"

#include "core/application.h"
#include "mainwindow.h"
#include "tray.h"
#include "window/main_window.h"
#include "window/window_controller.h"
#include "settings.h"

#include <QtCore/QFile>
#include <QtCore/QSaveFile>

#include <array>

namespace Nano {
namespace {

constexpr auto kNames = std::array<const char*, kAppIconsCount>{
	"classic",
	"glass",
	"blue",
	"green",
	"extera",
};

[[nodiscard]] QString StoragePath() {
	return cWorkingDir() + u"tdata/nanogram_icon"_q;
}

[[nodiscard]] AppIcon Read() {
	auto file = QFile(StoragePath());
	if (!file.open(QIODevice::ReadOnly)) {
		return AppIcon::Classic;
	}
	const auto name = file.readAll().trimmed();
	for (auto i = 0; i != kAppIconsCount; ++i) {
		if (name == kNames[i]) {
			return AppIcon(i);
		}
	}
	return AppIcon::Classic;
}

[[nodiscard]] AppIcon &Current() {
	static auto result = Read();
	return result;
}

[[nodiscard]] QImage Load(int index, const QString &suffix) {
	return QImage(u":/gui/art/nano/"_q
		+ QString::fromLatin1(kNames[index])
		+ suffix
		+ u".png"_q);
}

} // namespace

AppIcon CurrentAppIcon() {
	return Current();
}

const QImage &AppIconImage(AppIcon icon) {
	static auto cache = std::array<QImage, kAppIconsCount>();
	auto &result = cache[int(icon)];
	if (result.isNull()) {
		result = Load(int(icon), QString());
	}
	return result;
}

const QImage &AppIconImageSmall(AppIcon icon) {
	static auto cache = std::array<QImage, kAppIconsCount>();
	auto &result = cache[int(icon)];
	if (result.isNull()) {
		result = Load(int(icon), u"_small"_q);
	}
	return result;
}

void SetAppIcon(AppIcon icon) {
	if (Current() == icon) {
		return;
	}
	Current() = icon;

	auto file = QSaveFile(StoragePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(kNames[int(icon)]);
		if (!file.commit()) {
			LOG(("nanoGram Error: could not save the app icon."));
		}
	}

	Core::App().refreshApplicationIcon();
	Core::App().enumerateWindows([](not_null<Window::Controller*> window) {
		window->widget()->updateWindowIcon();
	});
	Core::App().tray().updateIconCounters();
}

} // namespace Nano
