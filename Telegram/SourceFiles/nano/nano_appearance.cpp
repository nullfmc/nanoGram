/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "nano/nano_appearance.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "main/main_session.h"
#include "main/main_session_settings.h"
#include "storage/localstorage.h"
#include "ui/effects/animation_value.h"
#include "ui/painter.h"
#include "window/themes/window_theme.h"
#include "window/themes/window_themes_embedded.h"
#include "settings.h"

#include "data/data_wall_paper.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>

namespace Nano {
namespace {

[[nodiscard]] QString StoragePath() {
	return cWorkingDir() + u"tdata/nanogram_userpic_shape"_q;
}

[[nodiscard]] UserpicShape &Shape() {
	static auto result = [] {
		auto file = QFile(StoragePath());
		if (file.open(QIODevice::ReadOnly)) {
			const auto value = file.readAll().trimmed().toInt();
			if (value > 0 && value < kUserpicShapesCount) {
				return UserpicShape(value);
			}
		}
		return UserpicShape::Circle;
	}();
	return result;
}

[[nodiscard]] Window::Theme::EmbeddedType CurrentType() {
	return Window::Theme::IsNightMode()
		? Window::Theme::EmbeddedType::Night
		: Window::Theme::EmbeddedType::Default;
}

} // namespace

UserpicShape CurrentUserpicShape() {
	return Shape();
}

void SetUserpicShape(UserpicShape shape) {
	if (Shape() == shape) {
		return;
	}
	Shape() = shape;
	auto file = QSaveFile(StoragePath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QByteArray::number(int(shape)));
		if (!file.commit()) {
			LOG(("nanoGram Error: could not save the userpic shape."));
		}
	}
	// userpic caches are checked against the palette version
	style::NotifyPaletteChanged();
}

QPainterPath UserpicShapePath(QRectF rect, UserpicShape shape) {
	auto result = QPainterPath();
	const auto size = rect.width();
	switch (shape) {
	case UserpicShape::Circle:
		result.addEllipse(rect);
		break;
	case UserpicShape::Squircle:
		result.addRoundedRect(rect, size * 0.3, size * 0.3);
		break;
	case UserpicShape::Flower: {
		// the same wavy shape as in the nanoGram logo
		constexpr auto kPoints = 120;
		constexpr auto kPetals = 7;
		const auto center = rect.center();
		const auto radius = size / 2. / 1.08;
		for (auto i = 0; i != kPoints; ++i) {
			const auto angle = 2 * M_PI * i / kPoints;
			const auto r = radius * (1 + 0.08 * std::cos(kPetals * angle));
			const auto point = QPointF(
				center.x() + r * std::sin(angle),
				center.y() - r * std::cos(angle));
			if (i) {
				result.lineTo(point);
			} else {
				result.moveTo(point);
			}
		}
		result.closeSubpath();
	} break;
	case UserpicShape::Drop: {
		const auto r = size / 2.;
		const auto small = size * 0.18;
		const auto left = rect.left();
		const auto top = rect.top();
		result.moveTo(left + r, top);
		result.lineTo(left + size - small, top);
		result.quadTo(left + size, top, left + size, top + small);
		result.lineTo(left + size, top + r);
		result.arcTo(QRectF(left, top, size, size), 0., -270.);
		result.closeSubpath();
	} break;
	}
	return result;
}

QPainterPath UserpicShapePath(QRectF rect) {
	return UserpicShapePath(rect, CurrentUserpicShape());
}

QImage ApplyUserpicShape(QImage image) {
	auto result = QImage(image.size(), QImage::Format_ARGB32_Premultiplied);
	result.setDevicePixelRatio(image.devicePixelRatio());
	result.fill(Qt::transparent);
	{
		auto p = QPainter(&result);
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(Qt::NoPen);
		p.setBrush(QBrush(image));
		p.drawPath(UserpicShapePath(QRectF(QPointF(), QSizeF(image.size()))));
	}
	return result;
}

std::vector<QColor> MaterialAccents() {
	return {
		QColor(0x76, 0x57, 0xd6), // violet
		QColor(0x3d, 0x7b, 0xe0), // blue
		QColor(0x0f, 0x9d, 0x9a), // teal
		QColor(0x3c, 0x9f, 0x5b), // green
		QColor(0xd1, 0x47, 0x7e), // pink
		QColor(0xe0, 0x7b, 0x2c), // orange
		QColor(0xd6, 0x45, 0x45), // red
		QColor(0x6b, 0x6a, 0x75), // graphite
	};
}

QColor CurrentAccent() {
	using namespace Window::Theme;
	const auto type = CurrentType();
	if (const auto color = Core::App().settings().themesAccentColors().get(type)) {
		return *color;
	}
	for (const auto &scheme : EmbeddedThemes()) {
		if (scheme.type == type) {
			return scheme.accentColor;
		}
	}
	return MaterialAccents().front();
}

void SetAccent(const QColor &color) {
	using namespace Window::Theme;
	const auto type = CurrentType();
	auto &colors = Core::App().settings().themesAccentColors();
	if (colors.get(type) == color) {
		return;
	}
	colors.set(type, color);
	Local::writeSettings();
	for (const auto &scheme : EmbeddedThemes()) {
		if (scheme.type == type) {
			ApplyDefaultWithPath(scheme.path);
			KeepApplied();
			break;
		}
	}
	RefreshDefaultWallpaper();
}

void ToggleDarkTheme(not_null<Window::Controller*> window) {
	Window::Theme::ToggleNightModeWithConfirmation(window, [] {
		Window::Theme::ToggleNightMode();
	});
}

namespace {

[[nodiscard]] QString ValuesPath() {
	return cWorkingDir() + u"tdata/nanogram_values"_q;
}

[[nodiscard]] base::flat_map<QByteArray, int> &Values() {
	static auto result = [] {
		auto values = base::flat_map<QByteArray, int>();
		auto file = QFile(ValuesPath());
		if (file.open(QIODevice::ReadOnly)) {
			for (const auto &line : file.readAll().split('\n')) {
				const auto parts = line.trimmed().split('=');
				if (parts.size() == 2) {
					values[parts[0]] = parts[1].toInt();
				}
			}
		}
		return values;
	}();
	return result;
}

[[nodiscard]] int Value(const char *key, int fallback) {
	const auto i = Values().find(QByteArray(key));
	return (i != end(Values())) ? i->second : fallback;
}

void SetValue(const char *key, int value) {
	Values()[QByteArray(key)] = value;
	auto data = QByteArray();
	for (const auto &[name, number] : Values()) {
		data += name + '=' + QByteArray::number(number) + '\n';
	}
	auto file = QSaveFile(ValuesPath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(data);
		if (!file.commit()) {
			LOG(("nanoGram Error: could not save values."));
		}
	}
}

} // namespace

std::vector<int> BubbleRadii() {
	return { 0, 6, 12, 20 };
}

int BubbleRadius() {
	const auto value = Value("bubble_radius", 0);
	const auto radii = BubbleRadii();
	return (ranges::find(radii, value) != end(radii)) ? value : 0;
}

void SetBubbleRadius(int radius) {
	SetValue("bubble_radius", radius);
}

bool BubbleTails() {
	return Value("bubble_tails", 1) != 0;
}

void SetBubbleTails(bool enabled) {
	SetValue("bubble_tails", enabled ? 1 : 0);
}

bool BubbleShadow() {
	return Value("bubble_shadow", 1) != 0;
}

void SetBubbleShadow(bool enabled) {
	SetValue("bubble_shadow", enabled ? 1 : 0);
	style::NotifyPaletteChanged();
}

bool StreamerMode() {
	return Value("streamer", 0) != 0;
}

void SetStreamerMode(bool enabled, not_null<Main::Session*> session) {
	if (StreamerMode() == enabled) {
		return;
	}
	auto &settings = session->settings();
	if (enabled) {
		// remember the choice of the user to bring it back later
		SetValue("streamer_phone_was_hidden", settings.phoneNumberHidden() ? 1 : 0);
		settings.setPhoneNumberHidden(true);
	} else {
		settings.setPhoneNumberHidden(
			Value("streamer_phone_was_hidden", 0) != 0);
	}
	session->saveSettingsDelayed();
	SetValue("streamer", enabled ? 1 : 0);
	style::NotifyPaletteChanged();
}

ChatStyle CurrentChatStyle() {
	return (Value("chat_style", 1) == 0) ? ChatStyle::Phone : ChatStyle::Concept;
}

void SetChatStyle(ChatStyle style) {
	SetValue("chat_style", int(style));
	if (style == ChatStyle::Concept) {
		SetBubbleRadius(20);
		SetBubbleTails(false);
		SetBubbleShadow(false);
	} else {
		SetBubbleRadius(0);
		SetBubbleTails(true);
		SetBubbleShadow(true);
	}
}

bool NavigationRail() {
	return Value("navigation_rail", 1) != 0;
}

void SetNavigationRail(bool enabled) {
	SetValue("navigation_rail", enabled ? 1 : 0);
}

std::vector<QColor> WallpaperColors() {
	const auto accent = CurrentAccent();
	const auto hue = std::max(accent.hslHue(), 0);
	const auto saturation = std::min(accent.hslSaturation(), 170);
	const auto make = [&](int lightness, int hueShift) {
		return QColor::fromHsl((hue + hueShift + 360) % 360, saturation, lightness);
	};
	return { make(226, -8), make(176, 6), make(214, 12), make(194, -4) };
}

void RefreshDefaultWallpaper() {
	using namespace Window::Theme;
	if (Data::IsDefaultWallPaper(Background()->paper())) {
		Background()->set(Data::DefaultWallPaper());
	}
}

namespace {

[[nodiscard]] QString NotesPath() {
	return cWorkingDir() + u"tdata/nanogram_notes.json"_q;
}

[[nodiscard]] QJsonObject &Notes() {
	static auto result = [] {
		auto file = QFile(NotesPath());
		return file.open(QIODevice::ReadOnly)
			? QJsonDocument::fromJson(file.readAll()).object()
			: QJsonObject();
	}();
	return result;
}

} // namespace

QString Note(uint64 peerId) {
	return Notes().value(QString::number(peerId)).toString();
}

void SetNote(uint64 peerId, const QString &text) {
	const auto key = QString::number(peerId);
	if (text.trimmed().isEmpty()) {
		Notes().remove(key);
	} else {
		Notes()[key] = text.trimmed();
	}
	auto file = QSaveFile(NotesPath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(Notes()).toJson(QJsonDocument::Compact));
		if (!file.commit()) {
			LOG(("nanoGram Error: could not save notes."));
		}
	}
}

AnimationMode CurrentAnimationMode() {
	const auto value = Value("animations", 0);
	return (value > 0 && value < kAnimationModesCount)
		? AnimationMode(value)
		: AnimationMode::Normal;
}

void SetAnimationMode(AnimationMode mode) {
	SetValue("animations", int(mode));
	ApplyAnimationMode();
}

void ApplyAnimationMode() {
	const auto mode = CurrentAnimationMode();
	anim::SetDisabled(mode == AnimationMode::Off);
	anim::SetSlowMultiplier((mode == AnimationMode::ExtraSmooth)
		? 3
		: (mode == AnimationMode::Smooth)
		? 2
		: 1);
}

} // namespace Nano
