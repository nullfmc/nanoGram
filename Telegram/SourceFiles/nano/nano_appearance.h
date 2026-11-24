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

// Phone: like Telegram on phones. Concept: like the nanoGram concept,
// round bubbles without tails and shadows, a pill shaped message field.
enum class ChatStyle {
	Phone,
	Concept,
};
[[nodiscard]] ChatStyle CurrentChatStyle();
void SetChatStyle(ChatStyle style);

// Chats, Contacts, Calls, Saved and Settings in the side bar.
[[nodiscard]] bool NavigationRail();
void SetNavigationRail(bool enabled);

// The default wallpaper follows the accent color.
[[nodiscard]] std::vector<QColor> WallpaperColors();
void RefreshDefaultWallpaper();

// Local notes about people and chats, nobody else sees them.
[[nodiscard]] QString Note(uint64 peerId);
void SetNote(uint64 peerId, const QString &text);

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
