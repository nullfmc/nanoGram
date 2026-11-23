/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

class PeerData;

namespace Nano {

enum class Option {
	MessageSeconds,
	ProfileId,
};

[[nodiscard]] bool Enabled(Option option);
void SetEnabled(Option option, bool enabled);

// Russian or English, cloud language packs don't have our keys.
[[nodiscard]] QString Text(const char *russian, const char *english);

[[nodiscard]] QString FormatMessageTime(const QTime &time);
[[nodiscard]] QString PeerIdText(not_null<PeerData*> peer);

} // namespace Nano
