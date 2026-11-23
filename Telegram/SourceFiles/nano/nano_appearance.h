/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Main {
class Session;
} // namespace Main

namespace Window {
class Controller;
} // namespace Window

namespace Nano {

enum class UserpicShape {
	Circle,
	Squircle,
	Flower,
	Drop,
};

inline constexpr auto kUserpicShapesCount = 4;

[[nodiscard]] UserpicShape CurrentUserpicShape();
void SetUserpicShape(UserpicShape shape);

[[nodiscard]] QPainterPath UserpicShapePath(QRectF rect, UserpicShape shape);
[[nodiscard]] QPainterPath UserpicShapePath(QRectF rect);

// Masks a square image with the chosen shape.
[[nodiscard]] QImage ApplyUserpicShape(QImage image);

[[nodiscard]] std::vector<QColor> MaterialAccents();
[[nodiscard]] QColor CurrentAccent();
void SetAccent(const QColor &color);

void ToggleDarkTheme(not_null<Window::Controller*> window);

// Bubbles: radius 0 means the default one, applied after a restart.
[[nodiscard]] std::vector<int> BubbleRadii();
[[nodiscard]] int BubbleRadius();
void SetBubbleRadius(int radius);
[[nodiscard]] bool BubbleTails();
void SetBubbleTails(bool enabled);
[[nodiscard]] bool BubbleShadow();
void SetBubbleShadow(bool enabled);

// Hides chat previews, notification texts and the phone number.
[[nodiscard]] bool StreamerMode();
void SetStreamerMode(bool enabled, not_null<Main::Session*> session);

enum class AnimationMode {
	Normal,
	Smooth,
	ExtraSmooth,
	Off,
};
inline constexpr auto kAnimationModesCount = 4;
[[nodiscard]] AnimationMode CurrentAnimationMode();
void SetAnimationMode(AnimationMode mode);
void ApplyAnimationMode();

} // namespace Nano
