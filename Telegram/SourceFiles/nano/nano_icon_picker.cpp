/*
This file is part of nanoGram,
an unofficial fork of Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "nano/nano_icon_picker.h"

#include "nano/nano_app_icon.h"
#include "lang/lang_instance.h"
#include "ui/abstract_button.h"
#include "ui/painter.h"
#include "ui/vertical_list.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "styles/style_basic.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

namespace Nano {
namespace {

constexpr auto kButtonSize = 60;
constexpr auto kRingWidth = 2;
constexpr auto kImageSkip = 5;
constexpr auto kPadding = 18;

} // namespace

void AddIconPicker(not_null<Ui::VerticalLayout*> container) {
	// cloud language packs don't have our keys
	const auto russian = Lang::GetInstance().id().startsWith(u"ru"_q);

	Ui::AddDivider(container);
	Ui::AddSkip(container);
	Ui::AddSubsectionTitle(container, rpl::single(russian
		? QString::fromUtf8("Иконка nanoGram")
		: u"nanoGram icon"_q));

	const auto size = style::ConvertScale(kButtonSize);
	const auto row = container->add(object_ptr<Ui::FixedHeightWidget>(
		container,
		size + style::ConvertScale(kImageSkip) * 2));

	auto buttons = std::vector<Ui::AbstractButton*>();
	for (auto i = 0; i != kAppIconsCount; ++i) {
		const auto icon = AppIcon(i);
		const auto button = Ui::CreateChild<Ui::AbstractButton>(row);
		button->resize(size, size);
		button->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(button);
			auto hq = PainterHighQualityEnabler(p);
			p.setRenderHint(QPainter::SmoothPixmapTransform);

			const auto full = QRectF(button->rect());
			if (CurrentAppIcon() == icon) {
				const auto line = style::ConvertScale(kRingWidth);
				const auto radius = full.width() * 0.27;
				p.setPen(QPen(st::windowBgActive->c, line));
				p.setBrush(Qt::NoBrush);
				p.drawRoundedRect(
					full.adjusted(line, line, -line, -line),
					radius,
					radius);
			}
			const auto skip = style::ConvertScale(kImageSkip);
			p.drawImage(
				full.adjusted(skip, skip, -skip, -skip),
				AppIconImage(icon));
		}, button->lifetime());
		button->setClickedCallback([=] {
			SetAppIcon(icon);
			for (const auto child : row->children()) {
				if (child->isWidgetType()) {
					static_cast<QWidget*>(child)->update();
				}
			}
		});
		button->show();
		buttons.push_back(button);
	}

	row->widthValue(
	) | rpl::on_next([=](int width) {
		const auto padding = style::ConvertScale(kPadding);
		const auto step = std::min(
			(width - 2 * padding - size) / (kAppIconsCount - 1),
			size + padding);
		auto left = padding;
		for (const auto button : buttons) {
			button->moveToLeft(left, 0, width);
			left += step;
		}
	}, row->lifetime());

	Ui::AddSkip(container);
}

} // namespace Nano
