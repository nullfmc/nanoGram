/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "nano/nano_settings.h"

#include "settings/settings_common_session.h"

#include "api/api_user_privacy.h"
#include "apiwrap.h"
#include "core/version.h"
#include "main/main_session.h"
#include "nano/nano_app_icon.h"
#include "nano/nano_appearance.h"
#include "nano/nano_icon_picker.h"
#include "nano/nano_options.h"
#include "settings/settings_common.h"
#include "ui/abstract_button.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/themes/window_theme.h"
#include "window/window_session_controller.h"
#include "styles/style_basic.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

namespace Nano {
namespace {

constexpr auto kSecretTaps = 5;
constexpr auto kSecretTapsWindow = crl::time(700);

class SettingsSection : public ::Settings::Section<SettingsSection> {
public:
	SettingsSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

class WaffleSection : public ::Settings::Section<WaffleSection> {
public:
	WaffleSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

};

void AddOption(
		not_null<Ui::VerticalLayout*> container,
		const QString &title,
		Option option) {
	const auto button = ::Settings::AddButtonWithIcon(
		container,
		rpl::single(title),
		st::settingsButtonNoIcon);
	button->toggleOn(rpl::single(Enabled(option)));
	button->toggledChanges(
	) | rpl::on_next([=](bool enabled) {
		SetEnabled(option, enabled);
	}, button->lifetime());
}

void AddLogo(
		not_null<Ui::VerticalLayout*> container,
		Fn<void()> secret) {
	const auto size = style::ConvertScale(96);
	const auto height = size + style::ConvertScale(92);
	const auto logo = container->add(object_ptr<Ui::AbstractButton>(container));
	logo->resize(logo->width(), height);
	logo->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(logo);
		auto hq = PainterHighQualityEnabler(p);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		const auto left = (logo->width() - size) / 2;
		const auto top = style::ConvertScale(16);
		p.drawImage(
			QRect(left, top, size, size),
			AppIconImage(CurrentAppIcon()));
		p.setPen(st::windowBoldFg);
		p.setFont(st::semiboldFont->f);
		auto name = st::semiboldFont->f;
		name.setPixelSize(style::ConvertScale(24));
		p.setFont(name);
		const auto nameTop = top + size + style::ConvertScale(10);
		p.drawText(
			QRect(0, nameTop, logo->width(), style::ConvertScale(32)),
			Qt::AlignCenter,
			u"nanoGram"_q);
		p.setPen(st::windowSubTextFg);
		p.setFont(st::normalFont->f);
		p.drawText(
			QRect(0, nameTop + style::ConvertScale(32), logo->width(), style::ConvertScale(24)),
			Qt::AlignCenter,
			u"alpha · "_q + QString::fromLatin1(AppVersionStr));
	}, logo->lifetime());

	struct State {
		int taps = 0;
		crl::time last = 0;
	};
	const auto state = logo->lifetime().make_state<State>();
	logo->setClickedCallback([=] {
		const auto now = crl::now();
		state->taps = (now - state->last < kSecretTapsWindow)
			? (state->taps + 1)
			: 1;
		state->last = now;
		if (state->taps >= kSecretTaps) {
			state->taps = 0;
			secret();
		}
	});
}

void AddDarkTheme(
		not_null<Ui::VerticalLayout*> container,
		not_null<Window::SessionController*> controller) {
	const auto button = ::Settings::AddButtonWithIcon(
		container,
		rpl::single(Text("Тёмная тема", "Dark theme")),
		st::settingsButtonNoIcon);
	button->toggleOn(Window::Theme::IsNightModeValue());
	button->toggledChanges(
	) | rpl::filter([](bool dark) {
		return dark != Window::Theme::IsNightMode();
	}) | rpl::on_next([=] {
		ToggleDarkTheme(&controller->window());
	}, button->lifetime());
}

void AddAccents(not_null<Ui::VerticalLayout*> container) {
	const auto colors = MaterialAccents();
	const auto size = style::ConvertScale(40);
	const auto skip = style::ConvertScale(22);
	const auto row = container->add(object_ptr<Ui::FixedHeightWidget>(
		container,
		size + style::ConvertScale(16)));
	auto buttons = std::vector<Ui::AbstractButton*>();
	for (const auto &color : colors) {
		const auto button = Ui::CreateChild<Ui::AbstractButton>(row);
		button->resize(size, size);
		button->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(button);
			auto hq = PainterHighQualityEnabler(p);
			const auto line = style::ConvertScale(3);
			p.setPen(Qt::NoPen);
			p.setBrush(color);
			if (CurrentAccent() == color) {
				p.drawEllipse(button->rect());
				p.setBrush(Qt::NoBrush);
				p.setPen(QPen(st::windowBg->c, line));
				p.drawEllipse(QRectF(button->rect()).adjusted(
					line * 1.5,
					line * 1.5,
					-line * 1.5,
					-line * 1.5));
			} else {
				p.drawEllipse(QRectF(button->rect()).adjusted(
					line,
					line,
					-line,
					-line));
			}
		}, button->lifetime());
		button->setClickedCallback([=] {
			SetAccent(color);
		});
		button->show();
		buttons.push_back(button);
	}
	style::PaletteChanged(
	) | rpl::on_next([=] {
		row->update();
		for (const auto button : buttons) {
			button->update();
		}
	}, row->lifetime());
	row->widthValue(
	) | rpl::on_next([=](int width) {
		const auto padding = st::boxRowPadding.left();
		const auto step = std::min(
			(width - 2 * padding - size) / int(buttons.size() - 1),
			size + skip);
		auto left = padding;
		for (const auto button : buttons) {
			button->moveToLeft(left, style::ConvertScale(8), width);
			left += step;
		}
	}, row->lifetime());
}

void AddShapes(not_null<Ui::VerticalLayout*> container) {
	const auto size = style::ConvertScale(54);
	const auto row = container->add(object_ptr<Ui::FixedHeightWidget>(
		container,
		size + style::ConvertScale(16)));
	auto buttons = std::vector<Ui::AbstractButton*>();
	for (auto i = 0; i != kUserpicShapesCount; ++i) {
		const auto shape = UserpicShape(i);
		const auto button = Ui::CreateChild<Ui::AbstractButton>(row);
		button->resize(size, size);
		button->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(button);
			auto hq = PainterHighQualityEnabler(p);
			const auto chosen = (CurrentUserpicShape() == shape);
			const auto inner = QRectF(button->rect()).adjusted(
				size * 0.12,
				size * 0.12,
				-size * 0.12,
				-size * 0.12);
			if (chosen) {
				p.setPen(QPen(st::windowBgActive->c, style::ConvertScale(2)));
				p.setBrush(Qt::NoBrush);
				p.drawRoundedRect(
					QRectF(button->rect()).adjusted(1, 1, -1, -1),
					size * 0.28,
					size * 0.28);
			}
			p.setPen(Qt::NoPen);
			p.setBrush(chosen ? st::windowBgActive->c : st::windowSubTextFg->c);
			p.drawPath(UserpicShapePath(inner, shape));
		}, button->lifetime());
		button->setClickedCallback([=] {
			SetUserpicShape(shape);
			for (const auto other : row->children()) {
				if (other->isWidgetType()) {
					static_cast<QWidget*>(other)->update();
				}
			}
		});
		button->show();
		buttons.push_back(button);
	}
	row->widthValue(
	) | rpl::on_next([=](int width) {
		const auto padding = st::boxRowPadding.left();
		auto left = padding;
		for (const auto button : buttons) {
			button->moveToLeft(left, style::ConvertScale(8), width);
			left += size + style::ConvertScale(18);
		}
	}, row->lifetime());
}

void AddToggle(
		not_null<Ui::VerticalLayout*> container,
		const QString &title,
		bool value,
		Fn<void(bool)> changed) {
	const auto button = ::Settings::AddButtonWithIcon(
		container,
		rpl::single(title),
		st::settingsButtonNoIcon);
	button->toggleOn(rpl::single(value));
	button->toggledChanges(
	) | rpl::on_next(std::move(changed), button->lifetime());
}

// A row that goes to the next value on every click.
void AddChoice(
		not_null<Ui::VerticalLayout*> container,
		const QString &title,
		std::vector<QString> names,
		int current,
		Fn<void(int)> changed) {
	const auto index = container->lifetime().make_state<rpl::variable<int>>(
		current);
	const auto button = ::Settings::AddButtonWithLabel(
		container,
		rpl::single(title),
		index->value() | rpl::map([=](int value) { return names[value]; }),
		st::settingsButtonNoIcon);
	button->setClickedCallback([=] {
		*index = (index->current() + 1) % int(names.size());
		changed(index->current());
	});
}

void AddBubbles(not_null<Ui::VerticalLayout*> container) {
	const auto radii = BubbleRadii();
	const auto current = int(ranges::find(radii, BubbleRadius()) - begin(radii));
	AddChoice(
		container,
		Text("Скругление пузырей", "Bubble corners"),
		{
			Text("Обычное", "Default"),
			Text("Острое", "Sharp"),
			Text("Мягкое", "Soft"),
			Text("Круглое", "Round"),
		},
		current,
		[=](int index) { SetBubbleRadius(radii[index]); });
	AddToggle(
		container,
		Text("Хвостики у пузырей", "Bubble tails"),
		BubbleTails(),
		[](bool enabled) { SetBubbleTails(enabled); });
	AddToggle(
		container,
		Text("Тень под пузырями", "Bubble shadow"),
		BubbleShadow(),
		[](bool enabled) { SetBubbleShadow(enabled); });
}

void AddAnimations(not_null<Ui::VerticalLayout*> container) {
	AddChoice(
		container,
		Text("Анимации", "Animations"),
		{
			Text("Обычные", "Default"),
			Text("Плавные", "Smooth"),
			Text("Очень плавные", "Extra smooth"),
			Text("Выключены", "Off"),
		},
		int(CurrentAnimationMode()),
		[](int index) { SetAnimationMode(AnimationMode(index)); });
}

void AddPrivacy(
		not_null<Ui::VerticalLayout*> container,
		not_null<Main::Session*> session) {
	using Privacy = Api::UserPrivacy;
	const auto privacy = &session->api().userPrivacy();
	const auto name = [](Privacy::Option option) {
		switch (option) {
		case Privacy::Option::Everyone: return Text("Все", "Everybody");
		case Privacy::Option::Contacts: return Text("Контакты", "Contacts");
		case Privacy::Option::CloseFriends:
			return Text("Близкие друзья", "Close friends");
		case Privacy::Option::Nobody: return Text("Никто", "Nobody");
		}
		return QString();
	};
	const auto add = [&](Privacy::Key key, const QString &title) {
		privacy->reload(key);
		const auto current = container->lifetime().make_state<
			rpl::variable<Privacy::Rule>>(privacy->value(key));
		const auto button = ::Settings::AddButtonWithLabel(
			container,
			rpl::single(title),
			current->value() | rpl::map([=](const Privacy::Rule &rule) {
				return name(rule.option);
			}),
			st::settingsButtonNoIcon);
		button->setClickedCallback([=] {
			// change only the main option, keep the exceptions
			auto rule = current->current();
			rule.option = (rule.option == Privacy::Option::Everyone)
				? Privacy::Option::Contacts
				: (rule.option == Privacy::Option::Contacts)
				? Privacy::Option::Nobody
				: Privacy::Option::Everyone;
			privacy->save(key, rule);
		});
	};
	add(Privacy::Key::LastSeen, Text("Время захода", "Last seen"));
	add(Privacy::Key::ProfilePhoto, Text("Фото профиля", "Profile photo"));
	add(Privacy::Key::PhoneNumber, Text("Номер телефона", "Phone number"));
	add(Privacy::Key::Forwards, Text("Пересылка сообщений", "Forwarded messages"));
	add(Privacy::Key::Calls, Text("Звонки", "Calls"));
}

SettingsSection::SettingsSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> SettingsSection::title() {
	return rpl::single(Text("Настройки nanoGram", "nanoGram settings"));
}

void SettingsSection::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	AddLogo(content, [=] { showOther(WaffleSection::Id()); });
	Ui::AddDivider(content);

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		rpl::single(Text("Оформление", "Appearance")));
	AddDarkTheme(content, controller());
	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		rpl::single(Text("Акцент", "Accent")));
	AddAccents(content);
	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		rpl::single(Text("Форма аватарок", "Profile photo shape")));
	AddShapes(content);
	Ui::AddSkip(content);
	AddIconPicker(content);

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		rpl::single(Text("Пузыри сообщений", "Message bubbles")));
	AddBubbles(content);
	AddAnimations(content);
	Ui::AddSkip(content);
	Ui::AddDividerText(content, rpl::single(Text(
		"Скругление и хвостики применятся после перезапуска. "
		"Плавные анимации идут медленнее обычных.",
		"Corners and tails apply after a restart. "
		"Smooth animations are slower than the default ones.")));

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		rpl::single(Text("Режим стримера", "Streamer mode")));
	const auto session = &controller()->session();
	AddToggle(
		content,
		Text("Режим стримера", "Streamer mode"),
		StreamerMode(),
		[=](bool enabled) { SetStreamerMode(enabled, session); });
	Ui::AddSkip(content);
	Ui::AddDividerText(content, rpl::single(Text(
		"Для записи экрана и стримов: прячет текст сообщений в списке чатов, "
		"имена и текст в уведомлениях и твой номер телефона. "
		"Открытые чаты он не прячет.",
		"For screen recording: hides message previews in the chat list, "
		"names and texts in notifications and your phone number. "
		"Opened chats stay visible.")));

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		rpl::single(Text("Приватность в один клик", "One-click privacy")));
	AddPrivacy(content, &controller()->session());
	Ui::AddSkip(content);
	Ui::AddDividerText(content, rpl::single(Text(
		"Это настоящие настройки Telegram, они меняются на всех устройствах. "
		"Если скрыть время захода от всех, ты тоже не увидишь его у других.",
		"These are real Telegram settings, they change on all devices. "
		"If you hide your last seen time, you won't see it for others.")));

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		rpl::single(Text("Функции", "Features")));
	AddOption(
		content,
		Text("Секунды во времени сообщений", "Seconds in message time"),
		Option::MessageSeconds);
	AddOption(
		content,
		Text("ID в профиле", "ID in profiles"),
		Option::ProfileId);
	Ui::AddSkip(content);
	Ui::AddDividerText(content, rpl::single(Text(
		"Открой чат или профиль заново, чтобы увидеть изменения. "
		"В уже загруженных сообщениях они появятся после перезапуска.",
		"Reopen a chat or profile to see the changes. "
		"Loaded messages will update after a restart.")));

	Ui::ResizeFitChild(this, content);
}

WaffleSection::WaffleSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	const auto content = Ui::CreateChild<Ui::RpWidget>(this);
	content->resize(width(), style::ConvertScale(560));
	content->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(content);
		auto hq = PainterHighQualityEnabler(p);
		const auto side = std::min(
			content->width() - style::ConvertScale(80),
			style::ConvertScale(400));
		const auto left = (content->width() - side) / 2.;
		const auto top = style::ConvertScale(36);
		const auto rect = QRectF(left, top, side, side);
		const auto radius = side * 0.12;

		// a huge waffle
		auto shadow = QColor(0x7a, 0x4a, 0x10, 60);
		p.setPen(Qt::NoPen);
		p.setBrush(shadow);
		p.drawRoundedRect(rect.translated(0, side * 0.04), radius, radius);
		auto gradient = QLinearGradient(rect.topLeft(), rect.bottomRight());
		gradient.setColorAt(0., QColor(0xf6, 0xc9, 0x6a));
		gradient.setColorAt(1., QColor(0xd8, 0x93, 0x34));
		p.setBrush(gradient);
		p.drawRoundedRect(rect, radius, radius);

		constexpr auto kCells = 5;
		const auto border = side * 0.07;
		const auto gap = side * 0.05;
		const auto cell = (side - 2 * border - (kCells - 1) * gap) / kCells;
		for (auto row = 0; row != kCells; ++row) {
			for (auto column = 0; column != kCells; ++column) {
				const auto pit = QRectF(
					left + border + column * (cell + gap),
					top + border + row * (cell + gap),
					cell,
					cell);
				auto inner = QLinearGradient(pit.topLeft(), pit.bottomLeft());
				inner.setColorAt(0., QColor(0xb0, 0x6c, 0x1c));
				inner.setColorAt(1., QColor(0xd9, 0x9a, 0x3e));
				p.setBrush(inner);
				p.drawRoundedRect(pit, cell * 0.22, cell * 0.22);
			}
		}

		auto font = st::semiboldFont->f;
		font.setPixelSize(style::ConvertScale(44));
		p.setFont(font);
		p.setPen(st::windowBoldFg);
		p.drawText(
			QRectF(0, top + side + style::ConvertScale(30), content->width(), style::ConvertScale(70)),
			Qt::AlignCenter,
			QString::fromUtf8("фривафлЭ"));
	}, content->lifetime());
	widthValue(
	) | rpl::on_next([=](int width) {
		content->resize(width, content->height());
	}, content->lifetime());
	Ui::ResizeFitChild(this, content);
}

rpl::producer<QString> WaffleSection::title() {
	return rpl::single(QString::fromUtf8("Вафельность"));
}

} // namespace

::Settings::Type SettingsId() {
	return SettingsSection::Id();
}

} // namespace Nano
