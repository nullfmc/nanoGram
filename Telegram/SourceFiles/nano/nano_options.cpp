/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "nano/nano_options.h"

#include "data/data_peer.h"
#include "lang/lang_instance.h"
#include "settings.h"

#include <QtCore/QFile>
#include <QtCore/QSaveFile>

#include <array>

namespace Nano {
namespace {

constexpr auto kCount = 2;
constexpr auto kNames = std::array<const char*, kCount>{
	"message_seconds",
	"profile_id",
};

[[nodiscard]] QString StoragePath() {
	return cWorkingDir() + u"tdata/nanogram_options"_q;
}

[[nodiscard]] std::array<bool, kCount> &Values() {
	static auto result = [] {
		auto values = std::array<bool, kCount>{ { true, true } };
		auto file = QFile(StoragePath());
		if (file.open(QIODevice::ReadOnly)) {
			const auto data = file.readAll();
			for (auto i = 0; i != kCount; ++i) {
				values[i] = !data.contains(QByteArray(kNames[i]) + "=0");
			}
		}
		return values;
	}();
	return result;
}

} // namespace

bool Enabled(Option option) {
	return Values()[int(option)];
}

void SetEnabled(Option option, bool enabled) {
	Values()[int(option)] = enabled;

	auto data = QByteArray();
	for (auto i = 0; i != kCount; ++i) {
		data += QByteArray(kNames[i]) + (Values()[i] ? "=1\n" : "=0\n");
	}
	auto file = QSaveFile(StoragePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(data);
		if (!file.commit()) {
			LOG(("nanoGram Error: could not save options."));
		}
	}
}

QString Text(const char *russian, const char *english) {
	return QString::fromUtf8(Lang::GetInstance().id().startsWith(u"ru"_q)
		? russian
		: english);
}

QString FormatMessageTime(const QTime &time) {
	const auto locale = QLocale();
	if (!Enabled(Option::MessageSeconds)) {
		return locale.toString(time, QLocale::ShortFormat);
	}
	auto format = locale.timeFormat(QLocale::ShortFormat);
	if (!format.contains(u"ss"_q)) {
		format.replace(u"mm"_q, u"mm:ss"_q);
	}
	return locale.toString(time, format);
}

QString PeerIdText(not_null<PeerData*> peer) {
	// the same format as in the Bot API
	if (peerIsUser(peer->id)) {
		return QString::number(peerToUser(peer->id).bare);
	} else if (peerIsChat(peer->id)) {
		return QString::number(-qint64(peerToChat(peer->id).bare));
	}
	return QString::number(
		-1000000000000LL - qint64(peerToChannel(peer->id).bare));
}

} // namespace Nano
