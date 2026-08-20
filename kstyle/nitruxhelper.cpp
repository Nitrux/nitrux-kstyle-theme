/*
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nitruxhelper.h"

#include "nitrux.h"
#include "nitruxpropertynames.h"

#include <KColorScheme>
#include <KConfigGroup>
#include <KColorUtils>
#include <KIconLoader>
#include <qobject.h>

#include <QApplication>
#include <QDockWidget>
#include <QFileInfo>
#include <QMainWindow>
#include <QMdiArea>
#include <QMenuBar>
#include <QPainter>
#include <QStyleOption>
#include <QTreeView>

#include <QDialog>
#include <algorithm>

namespace Nitrux
{
//* contrast for arrow and treeline rendering
static const qreal arrowShade = 0.15;

static const qreal highlightBackgroundAlpha = Metrics::Blend_Value;

static const auto radioCheckSunkenDarkeningFactor = 110;

PaletteChangedEventFilter::PaletteChangedEventFilter(Helper *helper)
    : QObject(helper)
    , _helper(helper)
{
}

bool PaletteChangedEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::ApplicationPaletteChange || watched != qApp) {
        return QObject::eventFilter(watched, event);
    }
    if (!qApp->property("KDE_COLOR_SCHEME_PATH").isValid()) {
        return QObject::eventFilter(watched, event);
    }
    const auto path = qApp->property("KDE_COLOR_SCHEME_PATH").toString();
    if (!path.isEmpty()) {
        KConfig config(path, KConfig::SimpleConfig);
        KConfigGroup group(config.group(QStringLiteral("WM")));
        const QPalette palette(QApplication::palette());
        _helper->_activeTitleBarColor = group.readEntry("activeBackground", palette.color(QPalette::Active, QPalette::Highlight));
        _helper->_activeTitleBarTextColor = group.readEntry("activeForeground", palette.color(QPalette::Active, QPalette::HighlightedText));
        _helper->_inactiveTitleBarColor = group.readEntry("inactiveBackground", palette.color(QPalette::Disabled, QPalette::Highlight));
        _helper->_inactiveTitleBarTextColor = group.readEntry("inactiveForeground", palette.color(QPalette::Disabled, QPalette::HighlightedText));
    }
    return QObject::eventFilter(watched, event);
}

//____________________________________________________________________
Helper::Helper(KSharedConfig::Ptr config)
    : QObject()
    , _config(std::move(config))
    , _kwinConfig(KSharedConfig::openConfig("kwinrc"))
    , _eventFilter(new PaletteChangedEventFilter(this))
{
}

//____________________________________________________________________
KSharedConfig::Ptr Helper::config() const
{
    return _config;
}

//____________________________________________________________________
void Helper::loadConfig()
{
    _viewFocusBrush = KStatefulBrush(KColorScheme::View, KColorScheme::FocusColor);
    _viewHoverBrush = KStatefulBrush(KColorScheme::View, KColorScheme::HoverColor);
    _viewNegativeTextBrush = KStatefulBrush(KColorScheme::View, KColorScheme::NegativeText);
    _viewNeutralTextBrush = KStatefulBrush(KColorScheme::View, KColorScheme::NeutralText);

    const QPalette palette(QApplication::palette());
    _config->reparseConfiguration();
    _kwinConfig->reparseConfiguration();
    _cachedAutoValid = false;

    KConfigGroup globalGroup(_config->group(QStringLiteral("WM")));
    _activeTitleBarColor = globalGroup.readEntry("activeBackground", palette.color(QPalette::Active, QPalette::Highlight));
    _activeTitleBarTextColor = globalGroup.readEntry("activeForeground", palette.color(QPalette::Active, QPalette::HighlightedText));
    _inactiveTitleBarColor = globalGroup.readEntry("inactiveBackground", palette.color(QPalette::Disabled, QPalette::Highlight));
    _inactiveTitleBarTextColor = globalGroup.readEntry("inactiveForeground", palette.color(QPalette::Disabled, QPalette::HighlightedText));
    _contrastBias = KColorScheme::frameContrast();

    if (const QString colorSchemePath = qApp->property("KDE_COLOR_SCHEME_PATH").toString(); !colorSchemePath.isEmpty()) {
        KConfig config(colorSchemePath, KConfig::SimpleConfig);
        KConfigGroup appGroup(config.group(QStringLiteral("WM")));
        _activeTitleBarColor = appGroup.readEntry("activeBackground", _activeTitleBarColor);
        _activeTitleBarTextColor = appGroup.readEntry("activeForeground", _activeTitleBarTextColor);
        _inactiveTitleBarColor = appGroup.readEntry("inactiveBackground", _inactiveTitleBarColor);
        _inactiveTitleBarTextColor = appGroup.readEntry("inactiveForeground", _inactiveTitleBarTextColor);
    }
}

void Helper::installEventFilter(QApplication *app) const
{
    if (app) {
        app->installEventFilter(_eventFilter);
    }
}

void Helper::removeEventFilter(QApplication *app) const
{
    if (app) {
        app->removeEventFilter(_eventFilter);
    }
}

QColor transparentize(const QColor &color, qreal amount)
{
    auto clone = color;
    clone.setAlphaF(amount);
    return clone;
}

//____________________________________________________________________
QColor Helper::frameOutlineColor(const QPalette &palette, bool mouseOver, bool hasFocus, qreal opacity, AnimationMode mode) const
{
    QColor outline(KColorUtils::mix(palette.color(QPalette::Window), palette.color(QPalette::WindowText), frameIntensityBias()));

    // focus takes precedence over hover
    if (mode == AnimationFocus) {
        const QColor focus(focusColor(palette));
        const QColor hover(hoverColor(palette));

        if (mouseOver) {
            outline = KColorUtils::mix(hover, focus, opacity);
        } else {
            outline = KColorUtils::mix(outline, focus, opacity);
        }

    } else if (hasFocus) {
        outline = focusColor(palette);

    } else if (mode == AnimationHover) {
        const QColor hover(hoverColor(palette));
        outline = KColorUtils::mix(outline, hover, opacity);

    } else if (mouseOver) {
        outline = hoverColor(palette);
    }

    return outline;
}

//____________________________________________________________________
QColor Helper::focusOutlineColor(const QPalette &palette) const
{
    return KColorUtils::mix(focusColor(palette), palette.color(QPalette::WindowText), 0.15);
}

//____________________________________________________________________
QColor Helper::hoverOutlineColor(const QPalette &palette) const
{
    return KColorUtils::mix(hoverColor(palette), palette.color(QPalette::WindowText), 0.15);
}

//____________________________________________________________________
QColor Helper::sidePanelOutlineColor(const QPalette &palette, bool hasFocus, qreal opacity, AnimationMode mode) const
{
    QColor outline(palette.color(QPalette::Inactive, QPalette::Highlight));
    const QColor &focus = palette.color(QPalette::Active, QPalette::Highlight);

    if (mode == AnimationFocus) {
        outline = KColorUtils::mix(outline, focus, opacity);

    } else if (hasFocus) {
        outline = focus;
    }

    return outline;
}

//____________________________________________________________________
QColor Helper::frameBackgroundColor(const QPalette &palette, QPalette::ColorGroup group) const
{
    return KColorUtils::mix(palette.color(group, QPalette::Window), palette.color(group, QPalette::Base), Metrics::Blend_Value);
}

//____________________________________________________________________
QColor Helper::arrowColor(const QPalette &palette, QPalette::ColorGroup group, QPalette::ColorRole role) const
{
    switch (role) {
    case QPalette::Text:
        return KColorUtils::mix(palette.color(group, QPalette::Text), palette.color(group, QPalette::Base), arrowShade);
    case QPalette::WindowText:
        return KColorUtils::mix(palette.color(group, QPalette::WindowText), palette.color(group, QPalette::Window), arrowShade);
    case QPalette::ButtonText:
        return KColorUtils::mix(palette.color(group, QPalette::ButtonText), palette.color(group, QPalette::Button), arrowShade);
    default:
        return palette.color(group, role);
    }
}

//____________________________________________________________________
QColor Helper::arrowColor(const QPalette &palette, bool mouseOver, bool hasFocus, qreal opacity, AnimationMode mode) const
{
    QColor outline(arrowColor(palette, QPalette::WindowText));
    if (mode == AnimationHover) {
        const QColor focus(focusColor(palette));
        const QColor hover(hoverColor(palette));
        if (hasFocus) {
            outline = KColorUtils::mix(focus, hover, opacity);
        } else {
            outline = KColorUtils::mix(outline, hover, opacity);
        }

    } else if (mouseOver) {
        outline = hoverColor(palette);

    } else if (mode == AnimationFocus) {
        const QColor focus(focusColor(palette));
        outline = KColorUtils::mix(outline, focus, opacity);

    } else if (hasFocus) {
        outline = focusColor(palette);
    }

    return outline;
}

//____________________________________________________________________
QColor Helper::sliderOutlineColor(const QPalette &palette, bool mouseOver, bool hasFocus, qreal opacity, AnimationMode mode) const
{
    QColor outline(KColorUtils::mix(palette.color(QPalette::Button), palette.color(QPalette::ButtonText), frameIntensityBias()));

    // hover takes precedence over focus
    if (mode == AnimationHover) {
        const QColor hover(hoverColor(palette));
        const QColor focus(focusColor(palette));
        if (hasFocus) {
            outline = KColorUtils::mix(focus, hover, opacity);
        } else {
            outline = KColorUtils::mix(outline, hover, opacity);
        }

    } else if (mouseOver) {
        outline = hoverColor(palette);

    } else if (mode == AnimationFocus) {
        const QColor focus(focusColor(palette));
        outline = KColorUtils::mix(outline, focus, opacity);

    } else if (hasFocus) {
        outline = focusColor(palette);
    }

    return outline;
}

//____________________________________________________________________
QColor Helper::scrollBarHandleColor(const QPalette &palette, bool mouseOver, bool hasFocus, qreal opacity, AnimationMode mode, bool pressed) const
{
    const QColor highlight(palette.color(QPalette::Highlight));
    QColor color(KColorUtils::mix(palette.color(QPalette::AlternateBase), palette.color(QPalette::Text), 0.2));

    // MauiKit uses the regular highlight color for hovered and pressed handles.
    // Pressed state takes precedence over animation so it is never darkened.
    if (pressed) {
        return highlight;
    }

    // hover takes precedence over focus
    if (mode == AnimationHover) {
        const QColor focus(focusColor(palette));
        color = hasFocus ? KColorUtils::mix(focus, highlight, opacity) : KColorUtils::mix(color, highlight, opacity);
    } else if (mouseOver) {
        color = highlight;
    } else if (mode == AnimationFocus) {
        color = KColorUtils::mix(color, focusColor(palette), opacity);
    } else if (hasFocus) {
        color = focusColor(palette);
    }

    return color;
}

//______________________________________________________________________________
QColor Helper::checkBoxIndicatorColor(const QPalette &palette, bool mouseOver, bool active, qreal opacity, AnimationMode mode) const
{
    QColor color(KColorUtils::mix(palette.color(QPalette::Window), palette.color(QPalette::WindowText), 0.6));
    if (mode == AnimationHover) {
        const QColor focus(focusColor(palette));
        const QColor hover(hoverColor(palette));
        if (active) {
            color = KColorUtils::mix(focus, hover, opacity);
        } else {
            color = KColorUtils::mix(color, hover, opacity);
        }

    } else if (mouseOver) {
        color = hoverColor(palette);

    } else if (active) {
        color = focusColor(palette);
    }

    return color;
}

//______________________________________________________________________________
QColor Helper::separatorColor(const QPalette &palette) const
{
    return KColorUtils::mix(palette.color(QPalette::Window), palette.color(QPalette::WindowText), frameIntensityBias());
}

//______________________________________________________________________________
QPalette Helper::disabledPalette(const QPalette &source, qreal ratio) const
{
    QPalette copy(source);

    const QList<QPalette::ColorRole> roles =
        {QPalette::Window, QPalette::Highlight, QPalette::WindowText, QPalette::ButtonText, QPalette::Text, QPalette::Button};
    for (const QPalette::ColorRole &role : roles) {
        copy.setColor(role, KColorUtils::mix(source.color(QPalette::Active, role), source.color(QPalette::Disabled, role), 1.0 - ratio));
    }

    return copy;
}

//____________________________________________________________________
QColor Helper::alphaColor(QColor color, qreal alpha) const
{
    if (alpha >= 0 && alpha < 1.0) {
        color.setAlphaF(alpha * color.alphaF());
    }
    return color;
}

//______________________________________________________________________________
void Helper::renderDebugFrame(QPainter *painter, const QRectF &rect) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::red);
    painter->drawRect(strokedRect(rect));
    painter->restore();
}

//______________________________________________________________________________
void Helper::renderFocusRect(QPainter *painter, const QRectF &rect, const QBrush &brush, const QColor &outline, Sides sides) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);
    painter->setBrush(brush);

    if (!(outline.isValid() && sides)) {
        painter->setPen(Qt::NoPen);
        painter->drawRect(rect);

    } else {
        painter->setClipRect(rect);

        QRectF copy(strokedRect(rect));

        // Keep compact highlights rounded without turning them into capsules.
        const qreal radius(qMin(frameRadius(PenWidth::Frame), qMin(copy.width(), copy.height()) * 0.25));
        if (!(sides & SideTop)) {
            copy.adjust(0, -radius, 0, 0);
        }
        if (!(sides & SideBottom)) {
            copy.adjust(0, 0, 0, radius);
        }
        if (!(sides & SideLeft)) {
            copy.adjust(-radius, 0, 0, 0);
        }
        if (!(sides & SideRight)) {
            copy.adjust(0, 0, radius, 0);
        }

        painter->setPen(outline);
        // painter->setBrush( Qt::NoBrush );
        painter->drawRoundedRect(copy, radius, radius);
    }

    painter->restore();
}

//______________________________________________________________________________
void Helper::renderFocusLine(QPainter *painter, const QRectF &rect, const QColor &color) const
{
    if (!color.isValid()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(color);

    painter->translate(0, 2);
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    painter->restore();
}

//______________________________________________________________________________
void Helper::renderFrameWithSides(QPainter *painter, const QRectF &rect, const QColor &color, Qt::Edges edges, const QColor &outline) const
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing);

    QRectF frameRect(rect);

    // set brush
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);

    // render
    painter->drawRect(frameRect);

    // set brush again
    painter->setBrush(Qt::NoBrush);
    painter->setPen(outline);

    // manually apply the effects of StrokedRect here but only to the edges with a frame
    if (edges & Qt::LeftEdge) {
        frameRect.adjust(0.5, 0.0, 0.0, 0.0);
    }
    if (edges & Qt::RightEdge) {
        frameRect.adjust(0.0, 0, -0.5, 0.0);
    }
    if (edges & Qt::TopEdge) {
        frameRect.adjust(0.0, 0.5, 0.0, 0.0);
    }
    if (edges & Qt::BottomEdge) {
        frameRect.adjust(0.0, 0.0, 0.0, -0.5);
    }

    // draw lines
    if (edges & Qt::LeftEdge) {
        painter->drawLine(QLineF(frameRect.topLeft(), frameRect.bottomLeft()));
    }
    if (edges & Qt::RightEdge) {
        painter->drawLine(QLineF(frameRect.topRight(), frameRect.bottomRight()));
    }
    if (edges & Qt::TopEdge) {
        painter->drawLine(QLineF(frameRect.topLeft(), frameRect.topRight()));
    }
    if (edges & Qt::BottomEdge) {
        painter->drawLine(QLineF(frameRect.bottomLeft(), frameRect.bottomRight()));
    }

    painter->restore();
}

//______________________________________________________________________________
void Helper::renderFrame(QPainter *painter, const QRectF &rect, const QColor &color, const QColor &outline) const
{
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF frameRect(rect);
    qreal radius(frameRadius(PenWidth::NoPen));

    // set pen
    if (outline.isValid()) {
        painter->setPen(outline);
        frameRect = strokedRect(frameRect);
        radius = frameRadiusForNewPenWidth(radius, PenWidth::Frame);

    } else {
        painter->setPen(Qt::NoPen);
    }

    // set brush
    if (color.isValid()) {
        painter->setBrush(color);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    // render
    painter->drawRoundedRect(frameRect, radius, radius);
}

//______________________________________________________________________________
void Helper::renderSidePanelFrame(QPainter *painter, const QRectF &rect, const QColor &outline, Side side) const
{
    // check color
    if (!outline.isValid()) {
        return;
    }

    // adjust rect
    QRectF frameRect(strokedRect(rect));

    // setup painter
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(outline);

    // render
    switch (side) {
    default:
    case SideLeft:
        painter->drawLine(frameRect.topRight(), frameRect.bottomRight());
        break;

    case SideTop:
        painter->drawLine(frameRect.topLeft(), frameRect.topRight());
        break;

    case SideRight:
        painter->drawLine(frameRect.topLeft(), frameRect.bottomLeft());
        break;

    case SideBottom:
        painter->drawLine(frameRect.bottomLeft(), frameRect.bottomRight());
        break;

    case AllSides: {
        const qreal radius(frameRadius(PenWidth::Frame));
        painter->drawRoundedRect(frameRect, radius, radius);
        break;
    }
    }
}

//______________________________________________________________________________
void Helper::renderMenuFrame(QPainter *painter, const QRectF &rect, const QColor &color, const QColor &outline, bool roundCorners, Qt::Edges seamlessEdges)
    const
{
    painter->save();

    // set brush
    if (color.isValid()) {
        painter->setBrush(color);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    // We simulate being able to independently adjust corner radii by
    // setting a clip region and then extending the rectangle beyond it.
    if (seamlessEdges != Qt::Edges()) {
        painter->setClipRect(rect);
    }

    if (roundCorners) {
        painter->setRenderHint(QPainter::Antialiasing);
        QRectF frameRect(rect);

        qreal radius(frameRadius());

        frameRect.adjust( //
            seamlessEdges.testFlag(Qt::LeftEdge) ? -radius : 0,
            seamlessEdges.testFlag(Qt::TopEdge) ? -radius : 0,
            seamlessEdges.testFlag(Qt::RightEdge) ? radius : 0,
            seamlessEdges.testFlag(Qt::BottomEdge) ? radius : 0);

        // set pen
        if (outline.isValid()) {
            painter->setPen(outline);
            frameRect = strokedRect(frameRect);
            radius = frameRadiusForNewPenWidth(radius, PenWidth::Frame);

        } else {
            painter->setPen(Qt::NoPen);
        }

        // render
        painter->drawRoundedRect(frameRect, radius, radius);

    } else {
        painter->setRenderHint(QPainter::Antialiasing, false);
        QRectF frameRect(rect);

        frameRect.adjust( //
            seamlessEdges.testFlag(Qt::LeftEdge) ? 1 : 0,
            seamlessEdges.testFlag(Qt::TopEdge) ? 1 : 0,
            seamlessEdges.testFlag(Qt::RightEdge) ? -1 : 0,
            seamlessEdges.testFlag(Qt::BottomEdge) ? -1 : 0);

        if (outline.isValid()) {
            painter->setPen(outline);
            frameRect.adjust(0, 0, -1, -1);

        } else {
            painter->setPen(Qt::NoPen);
        }

        painter->drawRect(frameRect);
    }

    painter->restore();
}

QRegion Helper::menuFrameRegion(const QMenu *widget)
{
    if (!widget) {
        return {};
    }

    const bool hasAlpha(hasAlphaChannel(widget));
    const auto seamlessEdges = menuSeamlessEdges(widget);
    const auto roundCorners = hasAlpha;

    if (roundCorners) {
        QRectF frameRect(widget->rect());

        qreal radius(frameRadius());

        frameRect.adjust( //
            seamlessEdges.testFlag(Qt::LeftEdge) ? -radius : 0,
            seamlessEdges.testFlag(Qt::TopEdge) ? -radius : 0,
            seamlessEdges.testFlag(Qt::RightEdge) ? radius : 0,
            seamlessEdges.testFlag(Qt::BottomEdge) ? radius : 0);

        // outline is always valid/drawn
        frameRect = strokedRect(frameRect);
        radius = frameRadiusForNewPenWidth(radius, PenWidth::Frame);

        QPainterPath path;
        path.addRoundedRect(frameRect, radius, radius);
        return QRegion(path.toFillPolygon().toPolygon()).intersected(widget->rect());
    }

    return QRegion(widget->rect());
}

//______________________________________________________________________________
void Helper::renderButtonFrame(QPainter *painter,
                               const QRectF &rect,
                               const QPalette &palette,
                               const QHash<QByteArray, bool> &stateProperties,
                               qreal bgAnimation,
                               qreal penAnimation) const
{
    Q_UNUSED(bgAnimation);
    Q_UNUSED(penAnimation);

    const bool enabled = stateProperties.value("enabled", true);
    const bool hovered = stateProperties.value("hovered");
    const bool down = stateProperties.value("down");
    const bool checked = stateProperties.value("checked");
    const bool flat = stateProperties.value("flat");
    const bool roundButton = stateProperties.value("roundButton");
    const bool toolButton = stateProperties.value("toolButton");

    // Flat controls remain transparent until they become interactive.
    if (flat && !hovered && !down && !checked) {
        return;
    }

    const auto group = enabled ? QPalette::Active : QPalette::Disabled;
    const QColor baseColor = palette.color(group, flat ? QPalette::Window : QPalette::Button);
    const QColor highlightColor = palette.color(group, QPalette::Highlight);
    QColor background = baseColor;
    if (enabled && (down || checked)) {
        background = highlightColor;
    } else if (enabled && hovered) {
        if (flat && toolButton) {
            background = palette.color(group, QPalette::Button);
            background.setAlphaF(background.alphaF() * Metrics::ToolButton_HoverOpacity);
        } else {
            background = baseColor.darker(110);
        }
    }

    const qreal configuredRadius = frameRadius(PenWidth::Frame);
    const qreal toolButtonRadius = qMin(configuredRadius,
                                        qMin(rect.width(), rect.height()) * Metrics::ToolButton_MaximumRadiusRatio);
    const qreal roundRadius = std::max(rect.width(), rect.height()) / 2;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(background);
    painter->setPen(Qt::NoPen);
    const qreal radius = roundButton ? roundRadius : (toolButton ? toolButtonRadius : configuredRadius);
    painter->drawRoundedRect(rect, radius, radius);
}

//______________________________________________________________________________
void Helper::renderToolBoxFrame(QPainter *painter, const QRectF &rect, int tabWidth, const QColor &outline) const
{
    if (!outline.isValid()) {
        return;
    }

    // round radius
    const qreal radius(frameRadius(PenWidth::Frame));
    const QSizeF cornerSize(2 * radius, 2 * radius);

    // if rect - tabwidth is even, need to increase tabWidth by 1 unit
    // for anti aliasing
    if (!((rect.toRect().width() - tabWidth) % 2)) {
        ++tabWidth;
    }

    // adjust rect for antialiasing
    QRectF baseRect(strokedRect(rect));

    // create path
    QPainterPath path;
    path.moveTo(0, baseRect.height() - 1);
    path.lineTo((baseRect.width() - tabWidth) / 2 - radius, baseRect.height() - 1);
    path.arcTo(QRectF(QPointF((baseRect.width() - tabWidth) / 2 - 2 * radius, baseRect.height() - 1 - 2 * radius), cornerSize), 270, 90);
    path.lineTo((baseRect.width() - tabWidth) / 2, radius);
    path.arcTo(QRectF(QPointF((baseRect.width() - tabWidth) / 2, 0), cornerSize), 180, -90);
    path.lineTo((baseRect.width() + tabWidth) / 2 - 1 - radius, 0);
    path.arcTo(QRectF(QPointF((baseRect.width() + tabWidth) / 2 - 1 - 2 * radius, 0), cornerSize), 90, -90);
    path.lineTo((baseRect.width() + tabWidth) / 2 - 1, baseRect.height() - 1 - radius);
    path.arcTo(QRectF(QPointF((baseRect.width() + tabWidth) / 2 - 1, baseRect.height() - 1 - 2 * radius), cornerSize), 180, 90);
    path.lineTo(baseRect.width() - 1, baseRect.height() - 1);

    // render
    painter->setRenderHints(QPainter::Antialiasing);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(outline);
    painter->translate(baseRect.topLeft());
    painter->drawPath(path);
}

//______________________________________________________________________________
void Helper::renderTabWidgetFrame(QPainter *painter, const QRectF &rect, const QColor &color, const QColor &outline, Corners corners) const
{
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF frameRect(rect.adjusted(1, 1, -1, -1));
    qreal radius(frameRadius(PenWidth::NoPen));

    // set pen
    if (outline.isValid()) {
        painter->setPen(outline);
        frameRect = strokedRect(frameRect);
        radius = frameRadiusForNewPenWidth(radius, PenWidth::Frame);

    } else {
        painter->setPen(Qt::NoPen);
    }

    // set brush
    if (color.isValid()) {
        painter->setBrush(color);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    // render
    QPainterPath path(roundedPath(frameRect, corners, radius));
    painter->drawPath(path);
}

//______________________________________________________________________________
void Helper::renderSeparator(QPainter *painter, const QRectF &rect, const QColor &color, bool vertical) const
{
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(color);

    if (vertical) {
        painter->translate(rect.width() / 2, 0);
        painter->drawLine(rect.topLeft(), rect.bottomLeft());

    } else {
        painter->translate(0, rect.height() / 2);
        painter->drawLine(rect.topLeft(), rect.topRight());
    }
}

//______________________________________________________________________________
void Helper::renderCheckBoxBackground(QPainter *painter,
                                      const QRectF &rect,
                                      const QPalette &palette,
                                      CheckBoxState state,
                                      bool neutalHighlight,
                                      bool sunken,
                                      qreal animation) const
{
    // setup painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    // copy rect
    QRectF frameRect(rect);
    frameRect.adjust(2, 2, -2, -2);
    frameRect = strokedRect(frameRect);

    auto transparent = neutalHighlight ? neutralText(palette) : palette.highlight().color();
    transparent.setAlphaF(highlightBackgroundAlpha);

    QBrush penBrush;
    if (neutalHighlight) {
        penBrush = neutralText(palette);
    } else if (state == CheckOn || state == CheckPartial) {
        penBrush = palette.highlight().color();
    } else {
        penBrush = separatorColor(palette);
    }
    painter->setPen(QPen(penBrush, PenWidth::Frame));

    const auto radius = qMax(frameRadius() - 1.0, 0.0);

    painter->setBrush(palette.button().color().darker(sunken ? radioCheckSunkenDarkeningFactor : 100));
    painter->drawRoundedRect(frameRect, radius, radius);

    if (state == CheckOff) {
        return;
    }

    painter->setBrush(transparent);
    if (state == CheckAnimated) {
        painter->setOpacity(animation);
    }
    painter->drawRoundedRect(frameRect, radius, radius);
}

//______________________________________________________________________________
void Helper::renderCheckBox(QPainter *painter,
                            const QRectF &rect,
                            const QPalette &palette,
                            bool mouseOver,
                            CheckBoxState state,
                            CheckBoxState target,
                            bool neutalHighlight,
                            bool sunken,
                            qreal animation,
                            qreal hoverAnimation) const
{
    Q_UNUSED(sunken)

    // setup painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    // copy rect and radius
    QRectF frameRect(rect);
    frameRect.adjust(2, 2, -2, -2);

    if (mouseOver) {
        painter->save();

        if (hoverAnimation != AnimationData::OpacityInvalid) {
            painter->setOpacity(hoverAnimation);
        }

        painter->setPen(QPen(neutalHighlight ? neutralText(palette).lighter() : focusColor(palette), PenWidth::Frame));
        painter->setBrush(Qt::NoBrush);

        painter->drawRoundedRect(frameRect.adjusted(0.5, 0.5, -0.5, -0.5), qMax(frameRadius() - 1.0, 0.0), qMax(frameRadius() - 1.0, 0.0));

        painter->restore();
    }

    // check
    auto leftPoint = frameRect.center();
    leftPoint.setX(frameRect.left() + 4);

    auto bottomPoint = frameRect.center();
    bottomPoint.setX(bottomPoint.x() - 1);
    bottomPoint.setY(frameRect.bottom() - 5);

    auto rightPoint = frameRect.center();
    rightPoint.setX(rightPoint.x() + 4.5);
    rightPoint.setY(frameRect.top() + 5.5);

    QPainterPath path;
    path.moveTo(leftPoint);
    path.lineTo(bottomPoint);
    path.lineTo(rightPoint);

    // dots
    auto centerDot = QRectF(frameRect.center(), QSize(2, 2));
    centerDot.adjust(-1, -1, -1, -1);
    auto leftDot = centerDot.adjusted(-4, 0, -4, 0);
    auto rightDot = centerDot.adjusted(4, 0, 4, 0);

    painter->setPen(Qt::transparent);
    painter->setBrush(Qt::transparent);

    auto checkPen = QPen(palette.text(), PenWidth::Frame * 2);
    checkPen.setJoinStyle(Qt::MiterJoin);

    switch (state) {
    case CheckOff:
        break;
    case CheckOn:
        painter->setPen(checkPen);
        painter->drawPath(path);
        break;
    case CheckPartial:
        painter->setBrush(palette.text());
        painter->drawRect(leftDot);
        painter->drawRect(centerDot);
        painter->drawRect(rightDot);
        break;
    case CheckAnimated:
        checkPen.setDashPattern({path.length() * animation, path.length()});

        switch (target) {
        case CheckOff:
            break;
        case CheckOn:
            painter->setPen(checkPen);
            painter->drawPath(path);
            break;
        case CheckPartial:
            if (animation >= 3.0 / 3.0) {
                painter->drawRect(rightDot);
            }
            if (animation >= 2.0 / 3.0) {
                painter->drawRect(centerDot);
            }
            if (animation >= 1.0 / 3.0) {
                painter->drawRect(leftDot);
            }
            break;
        case CheckAnimated:
            break;
        }
        break;
    }
}

//______________________________________________________________________________
void Helper::renderRadioButtonBackground(QPainter *painter,
                                         const QRectF &rect,
                                         const QPalette &palette,
                                         RadioButtonState state,
                                         bool neutalHighlight,
                                         bool sunken,
                                         qreal animation) const
{
    // setup painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    // copy rect
    QRectF frameRect(rect);
    frameRect.adjust(2, 2, -2, -2);
    frameRect.adjust(0.5, 0.5, -0.5, -0.5);

    auto transparent = neutalHighlight ? neutralText(palette) : palette.highlight().color();
    transparent.setAlphaF(highlightBackgroundAlpha);

    QBrush penBrush;
    if (neutalHighlight) {
        penBrush = neutralText(palette);
    } else if (state == RadioOn) {
        penBrush = palette.highlight().color();
    } else {
        penBrush = separatorColor(palette);
    }
    painter->setPen(QPen(penBrush, PenWidth::Frame));

    painter->setBrush(palette.button().color().darker(sunken ? radioCheckSunkenDarkeningFactor : 100));
    painter->drawEllipse(frameRect);

    if (state == RadioOff) {
        return;
    }

    painter->setBrush(transparent);
    if (state == RadioAnimated) {
        painter->setOpacity(animation);
    }
    painter->drawEllipse(frameRect);
}

//______________________________________________________________________________
void Helper::renderRadioButton(QPainter *painter,
                               const QRectF &rect,
                               const QPalette &palette,
                               bool mouseOver,
                               RadioButtonState state,
                               bool neutralHighlight,
                               bool sunken,
                               qreal animation,
                               qreal animationHover) const
{
    Q_UNUSED(sunken)

    // copy rect
    QRectF frameRect(rect);
    frameRect.adjust(1, 1, -1, -1);

    if (mouseOver) {
        painter->save();

        if (animationHover != AnimationData::OpacityInvalid) {
            painter->setOpacity(animationHover);
        }

        painter->setPen(QPen(neutralHighlight ? neutralText(palette).lighter() : focusColor(palette), PenWidth::Frame));
        painter->setBrush(Qt::NoBrush);

        const QRectF contentRect(frameRect.adjusted(1, 1, -1, -1).adjusted(0.5, 0.5, -0.5, -0.5));
        painter->drawEllipse(contentRect);

        painter->restore();
    }

    painter->setBrush(palette.text());
    painter->setPen(Qt::NoPen);

    const int radius = (std::min(frameRect.width(), frameRect.height()) - 12) / 2;
    const QPointF center = frameRect.center();

    // mark
    switch (state) {
    case RadioOn:
        painter->drawEllipse(center, radius, radius);
        break;
    case RadioAnimated: {
        const qreal animationRadius = radius * animation;
        painter->drawEllipse(center, animationRadius, animationRadius);
        break;
    }
    default:
        break;
    }
}

//______________________________________________________________________________
void Helper::renderSliderGroove(QPainter *painter, const QRectF &rect, const QColor &fg, const QColor &bg) const
{
    // setup painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRectF baseRect(rect);
    baseRect.adjust(0.5, 0.5, -0.5, -0.5);
    const qreal radius(qMin(frameRadius(), qMin(baseRect.width(), baseRect.height()) * 0.5));

    Q_UNUSED(bg);
    if (fg.isValid()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(fg.alpha() < 255 ? KColorUtils::overlayColors(bg, fg) : fg);
        painter->drawRoundedRect(baseRect, radius, radius);
    }
}

//______________________________________________________________________________
void Helper::renderDialGroove(QPainter *painter, const QRectF &rect, const QColor &fg, const QColor &bg, qreal first, qreal last) const
{
    // setup painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF baseRect(rect);

    // content
    if (fg.isValid()) {
        const qreal penWidth(Metrics::Slider_GrooveThickness);
        const QRectF grooveRect(rect.adjusted(penWidth / 2, penWidth / 2, -penWidth / 2, -penWidth / 2));

        // setup angles
        const int angleStart(first * 180 * 16 / M_PI);
        const int angleSpan((last - first) * 180 * 16 / M_PI);
        const QPen bgPen(fg, penWidth, Qt::SolidLine, Qt::RoundCap);
        const QPen fgPen(transparentize(KColorUtils::overlayColors(bg, alphaColor(fg, 0.5)), frameIntensityBias()), penWidth - 2, Qt::SolidLine, Qt::RoundCap);

        // setup pen
        if (angleSpan != 0) {
            painter->setPen(bgPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawArc(grooveRect, angleStart, angleSpan);
            painter->setPen(fgPen);
            painter->drawArc(grooveRect, angleStart, angleSpan);
        }
    }
}

//______________________________________________________________________________
void Helper::initSliderStyleOption(const QSlider *slider, QStyleOptionSlider *option) const
{
    option->initFrom(slider);
    option->subControls = QStyle::SC_None;
    option->activeSubControls = QStyle::SC_None;
    option->orientation = slider->orientation();
    option->maximum = slider->maximum();
    option->minimum = slider->minimum();
    option->tickPosition = slider->tickPosition();
    option->tickInterval = slider->tickInterval();
    option->upsideDown = (slider->orientation() == Qt::Horizontal) //
        ? (slider->invertedAppearance() != (option->direction == Qt::RightToLeft))
        : (!slider->invertedAppearance());
    option->direction = Qt::LeftToRight; // we use the upsideDown option instead
    option->sliderPosition = slider->sliderPosition();
    option->sliderValue = slider->value();
    option->singleStep = slider->singleStep();
    option->pageStep = slider->pageStep();
    if (slider->orientation() == Qt::Horizontal) {
        option->state |= QStyle::State_Horizontal;
    }
    // Can't fetch activeSubControls, because it's private API
}

//______________________________________________________________________________
QRectF Helper::pathForSliderHandleFocusFrame(QPainterPath &focusFramePath, const QRectF &rect, int hmargin, int vmargin) const
{
    // Mimics path and adjustments of renderSliderHandle
    QRectF frameRect(rect);
    frameRect.translate(hmargin, vmargin);
    frameRect.adjust(1, 1, -1, -1);
    frameRect = strokedRect(frameRect);
    focusFramePath.addEllipse(frameRect);
    frameRect.adjust(-hmargin, -vmargin, hmargin, vmargin);
    focusFramePath.addEllipse(frameRect);
    return frameRect;
}

//______________________________________________________________________________
void Helper::renderSliderHandle(QPainter *painter, const QRectF &rect, const QColor &color, const QColor &outline, bool sunken) const
{
    Q_UNUSED(sunken);

    // setup painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Keep the handle at its full geometry when no border is requested.
    QRectF frameRect(rect);

    // set pen
    if (outline.isValid()) {
        frameRect.adjust(1, 1, -1, -1);
        painter->setPen(QPen(outline, PenWidth::Frame));
        frameRect = strokedRect(frameRect);

    } else {
        painter->setPen(Qt::NoPen);
    }

    // set brush
    if (color.isValid()) {
        painter->setBrush(color);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    // render
    painter->drawEllipse(frameRect);
}

//______________________________________________________________________________
void Helper::renderProgressBarGroove(QPainter *painter, const QRectF &rect, const QColor &fg, const QColor &bg) const
{
    Q_UNUSED(bg);

    painter->setRenderHint(QPainter::Antialiasing, true);

    QRectF baseRect(rect);
    baseRect.adjust(0.5, 0.5, -0.5, -0.5);
    const qreal radius(qMin(frameRadius(), qMin(baseRect.width(), baseRect.height()) * 0.5));

    // MauiKit draws a separate, subdued track beneath the highlight fill.
    if (fg.isValid()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(fg);
        painter->drawRoundedRect(baseRect, radius, radius);
    }
}

//______________________________________________________________________________
void Helper::renderProgressBarContents(QPainter *painter, const QRectF &rect, const QColor &color) const
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRectF baseRect(rect);
    baseRect.adjust(0.5, 0.5, -0.5, -0.5);
    const qreal radius(qMin(frameRadius(), qMin(baseRect.width(), baseRect.height()) * 0.5));

    if (color.isValid()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawRoundedRect(baseRect, radius, radius);
    }
}

//______________________________________________________________________________
void Helper::renderProgressBarBusyContents(QPainter *painter,
                                           const QRectF &rect,
                                           const QColor &first,
                                           const QColor &second,
                                           bool horizontal,
                                           bool reverse,
                                           int progress) const
{
    // setup painter
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF baseRect(rect);
    const qreal radius(qMin(frameRadius(), qMin(baseRect.width(), baseRect.height()) * 0.5));

    Q_UNUSED(second);

    // Keep the MauiKit track visible and paint only the animated highlight.
    QPixmap pixmap(horizontal ? 2 * Metrics::ProgressBar_BusyIndicatorSize : 1, horizontal ? 1 : 2 * Metrics::ProgressBar_BusyIndicatorSize);
    pixmap.fill(Qt::transparent);
    if (horizontal) {
        QPainter painter(&pixmap);
        painter.setBrush(first);
        painter.setPen(Qt::NoPen);

        progress %= 2 * Metrics::ProgressBar_BusyIndicatorSize;
        if (reverse) {
            progress = 2 * Metrics::ProgressBar_BusyIndicatorSize - progress - 1;
        }
        painter.drawRect(QRect(0, 0, Metrics::ProgressBar_BusyIndicatorSize, 1).translated(progress, 0));

        if (progress > Metrics::ProgressBar_BusyIndicatorSize) {
            painter.drawRect(QRect(0, 0, Metrics::ProgressBar_BusyIndicatorSize, 1).translated(progress - 2 * Metrics::ProgressBar_BusyIndicatorSize, 0));
        }

    } else {
        QPainter painter(&pixmap);
        painter.setBrush(first);
        painter.setPen(Qt::NoPen);

        progress %= 2 * Metrics::ProgressBar_BusyIndicatorSize;
        progress = 2 * Metrics::ProgressBar_BusyIndicatorSize - progress - 1;
        painter.drawRect(QRect(0, 0, 1, Metrics::ProgressBar_BusyIndicatorSize).translated(0, progress));

        if (progress > Metrics::ProgressBar_BusyIndicatorSize) {
            painter.drawRect(QRect(0, 0, 1, Metrics::ProgressBar_BusyIndicatorSize).translated(0, progress - 2 * Metrics::ProgressBar_BusyIndicatorSize));
        }
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(pixmap);
    painter->drawRoundedRect(baseRect, radius, radius);
}

//______________________________________________________________________________
void Helper::renderScrollBarHandle(QPainter *painter, const QRectF &rect, const QColor &color) const
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF baseRect(rect);
    const qreal radius(0.5 * std::min({baseRect.width(), baseRect.height(), (qreal)Metrics::ScrollBar_SliderWidth}));

    // MauiKit scrollbars use a borderless rounded handle.
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawRoundedRect(baseRect, radius, radius);
}

//______________________________________________________________________________
void Helper::renderScrollBarBorder(QPainter *painter, const QRectF &rect, const QColor &color) const
{
    // content
    if (color.isValid()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawRect(rect);
    }
}

void Helper::renderStaticTabBarTab(QPainter *painter,
                                   const QRectF &rect,
                                   const QPalette &palette,
                                   const QHash<QByteArray, bool> &stateProperties,
                                   Corners corners,
                                   qreal animation) const
{
    Q_UNUSED(corners);
    renderMauiKitTabBarTab(painter, rect, palette, stateProperties, animation);
}

//______________________________________________________________________________
void Helper::renderMauiKitTabBarTab(QPainter *painter,
                                    const QRectF &rect,
                                    const QPalette &palette,
                                    const QHash<QByteArray, bool> &stateProperties,
                                    qreal animation) const
{
    const bool enabled = stateProperties.value("enabled", true);
    const bool hovered = stateProperties.value("hovered");
    const bool down = stateProperties.value("down");
    const bool selected = stateProperties.value("selected");
    const bool north = stateProperties.value("north");
    const bool south = stateProperties.value("south");
    const bool west = stateProperties.value("west");
    const bool east = stateProperties.value("east");
    const bool animated = animation != AnimationData::OpacityInvalid;

    const QColor barColor = palette.color(QPalette::Window);
    const QRectF tabRect = rect.adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = frameRadius();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // MauiKit tabs are flat: only the selected tab or an interactive tab gets
    // a surface; inactive tabs remain transparent against the tab bar.
    if (selected || (enabled && (hovered || down))) {
        QColor background = selected ? palette.color(QPalette::Button) : hoverColor(palette);
        if (!selected && animated) {
            background = KColorUtils::mix(barColor, background, animation);
        }
        if (!enabled) {
            background = KColorUtils::mix(barColor, background, 0.5);
        }

        painter->setPen(Qt::NoPen);
        painter->setBrush(background);
        painter->drawRoundedRect(tabRect, radius, radius);
    }

    // Root-status tabs receive a short, two-pixel strip on the content-facing
    // edge; ordinary selection and keyboard focus stay borderless.
    if (stateProperties.value("rootStatus")) {
        constexpr qreal indicatorThickness = 2.0;
        QRectF indicator(tabRect);
        if (north || south) {
            indicator.setWidth(tabRect.width() * 0.9);
            indicator.moveLeft(tabRect.center().x() - indicator.width() / 2.0);
            indicator.setHeight(indicatorThickness);
            if (north) {
                indicator.moveBottom(tabRect.bottom());
            } else {
                indicator.moveTop(tabRect.top());
            }
        } else if (west || east) {
            indicator.setHeight(tabRect.height() * 0.9);
            indicator.moveTop(tabRect.center().y() - indicator.height() / 2.0);
            indicator.setWidth(indicatorThickness);
            if (west) {
                indicator.moveRight(tabRect.right());
            } else {
                indicator.moveLeft(tabRect.left());
            }
        }

        QColor indicatorColor = negativeText(palette);
        if (!enabled) {
            indicatorColor = KColorUtils::mix(barColor, indicatorColor, 0.5);
        }
        painter->setBrush(indicatorColor);
        painter->drawRect(indicator);
    }

    painter->restore();
}

//______________________________________________________________________________
void Helper::renderTabBarTab(QPainter *painter,
                             const QRectF &rect,
                             const QPalette &palette,
                             const QHash<QByteArray, bool> &stateProperties,
                             Corners corners,
                             qreal animation) const
{
    Q_UNUSED(corners);
    renderMauiKitTabBarTab(painter, rect, palette, stateProperties, animation);
}

//______________________________________________________________________________
void Helper::renderArrow(QPainter *painter, const QRectF &rect, const QColor &color, ArrowOrientation orientation) const
{
    int size = std::min({rect.toRect().width(), rect.toRect().height(), Metrics::ArrowSize});
    // No point in trying to draw if it's too small
    if (size <= 0) {
        return;
    }

    qreal penOffset = PenWidth::Symbol / 2.0;
    qreal center = size / 2.0;
    qreal maxExtent = size * 0.75;
    qreal minExtent = size / 4.0;
    qreal sizeOffset = 0;
    int remainder = size % 4;
    if (remainder == 2) {
        sizeOffset = 0.5;
    } else if (remainder == 1) {
        sizeOffset = -0.25;
    } else if (remainder == 3) {
        sizeOffset = 0.25;
    }

    QPolygonF arrow;
    switch (orientation) {
    case ArrowUp:
        arrow = QVector<QPointF>{
            {penOffset, maxExtent - penOffset - sizeOffset}, // left
            {center, minExtent - sizeOffset}, // mid
            {size - penOffset, maxExtent - penOffset - sizeOffset} // right
        };
        break;
    case ArrowDown:
        arrow = QVector<QPointF>{
            {penOffset, minExtent + penOffset + sizeOffset}, // left
            {center, maxExtent + sizeOffset}, // mid
            {size - penOffset, minExtent + penOffset + sizeOffset} // right
        };
        break;
    case ArrowLeft:
        arrow = QVector<QPointF>{
            {maxExtent - penOffset - sizeOffset, penOffset}, // top
            {minExtent - sizeOffset, center}, // mid
            {maxExtent - penOffset - sizeOffset, size - penOffset}, // bottom
        };
        break;
    case ArrowRight:
        arrow = QVector<QPointF>{
            {minExtent + penOffset + sizeOffset, penOffset}, // top
            {maxExtent + sizeOffset, center}, // mid
            {minExtent + penOffset + sizeOffset, size - penOffset}, // bottom
        };
        break;
    default:
        break;
    }

    arrow.translate(rect.x() + (rect.width() - size) / 2.0, rect.y() + (rect.height() - size) / 2.0);

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);
    painter->setBrush(Qt::NoBrush);
    QPen pen(color, PenWidth::Symbol);
    pen.setCapStyle(Qt::SquareCap);
    pen.setJoinStyle(Qt::MiterJoin);
    painter->setPen(pen);
    painter->drawPolyline(arrow);
    painter->restore();
}

//______________________________________________________________________________
void Helper::renderTriangleArrow(QPainter *painter, const QRectF &rect, const QColor &color, ArrowOrientation orientation) const
{
    if ((orientation != ArrowUp && orientation != ArrowDown)
        || rect.width() < Metrics::TriangleArrow_Width || rect.height() < Metrics::TriangleArrow_Height) {
        return;
    }

    const QPointF center = rect.center();
    const qreal halfWidth = Metrics::TriangleArrow_Width * 0.5;
    const qreal halfHeight = Metrics::TriangleArrow_Height * 0.5;
    const qreal top = center.y() - halfHeight;
    const qreal bottom = center.y() + halfHeight;

    const QPolygonF triangle = orientation == ArrowUp
        ? QPolygonF{{center.x() - halfWidth, bottom}, {center.x(), top}, {center.x() + halfWidth, bottom}}
        : QPolygonF{{center.x() - halfWidth, top}, {center.x(), bottom}, {center.x() + halfWidth, top}};

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    painter->drawPolygon(triangle);
    painter->restore();
}

//______________________________________________________________________________
void Helper::renderDecorationButton(QPainter *painter, const QRectF &rect, const QColor &color, ButtonType buttonType, bool inverted) const
{
    painter->save();
    painter->setViewport(rect.toRect());
    painter->setWindow(0, 0, 18, 18);
    painter->setRenderHints(QPainter::Antialiasing);

    // initialize pen
    QPen pen;
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::MiterJoin);

    if (inverted) {
        // render circle
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawEllipse(QRectF(0, 0, 18, 18));

        // take out the inner part
        painter->setCompositionMode(QPainter::CompositionMode_DestinationOut);
        painter->setBrush(Qt::NoBrush);
        pen.setColor(Qt::black);

    } else {
        painter->setBrush(Qt::NoBrush);
        pen.setColor(color);
    }

    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setWidthF(PenWidth::Symbol * qMax(1.0, 18.0 / rect.width()));
    painter->setPen(pen);

    switch (buttonType) {
    case ButtonClose: {
        painter->drawLine(QPointF(5, 5), QPointF(13, 13));
        painter->drawLine(13, 5, 5, 13);
        break;
    }

    case ButtonMaximize: {
        painter->drawPolyline(QVector<QPointF>{QPointF(4, 11), QPointF(9, 6), QPointF(14, 11)});
        break;
    }

    case ButtonMinimize: {
        painter->drawPolyline(QVector<QPointF>{QPointF(4, 7), QPointF(9, 12), QPointF(14, 7)});
        break;
    }

    case ButtonRestore: {
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->drawPolygon(QVector<QPointF>{QPointF(4.5, 9), QPointF(9, 4.5), QPointF(13.5, 9), QPointF(9, 13.5)});
        break;
    }

    default:
        break;
    }

    painter->restore();
}

//______________________________________________________________________________
void Helper::renderViewItemPosition(QPainter *painter,
                                    const QStyleOptionViewItem::ViewItemPosition &pos,
                                    const Qt::LayoutDirection direction,
                                    const QRectF &rect,
                                    const QBrush &bg,
                                    const QColor &outline) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QFlags<Nitrux::Side> sides;
    const bool reverse = direction == Qt::RightToLeft;

    switch (pos) {
    case QStyleOptionViewItem::Invalid:
        painter->setBrush(bg);
        painter->setPen(outline);
        painter->drawRect(rect);
        painter->restore();
        return;
    case QStyleOptionViewItem::Beginning:
        painter->setClipping(true);
        painter->setClipRect(rect);
        sides = SideTop | SideBottom | (reverse ? SideRight : SideLeft);
        break;
    case QStyleOptionViewItem::Middle:;
        sides = SideTop | SideBottom;
        break;
    case QStyleOptionViewItem::End:
        painter->setClipping(true);
        painter->setClipRect(rect);
        sides = SideTop | SideBottom | (reverse ? SideLeft : SideRight);
        break;
    case QStyleOptionViewItem::OnlyOne:
        sides = SideTop | SideBottom | SideLeft | SideRight;
        break;
    }
    renderFocusRect(painter, rect, bg, outline, sides);
    painter->restore();
}

//______________________________________________________________________________
QRectF Helper::strokedRect(const QRectF &rect, const qreal penWidth) const
{
    /* With a pen stroke width of 1, the rectangle should have each of its
     * sides moved inwards by half a pixel. This allows the stroke to be
     * pixel perfect instead of blurry from sitting between pixels and
     * prevents the rectangle with a stroke from becoming larger than the
     * original size of the rectangle.
     */
    qreal adjustment = 0.5 * penWidth;
    return rect.adjusted(adjustment, adjustment, -adjustment, -adjustment);
}

QMargins Helper::itemViewItemMargins(const QStyleOptionViewItem *option) const
{
    QMargins margins(Metrics::ItemView_ItemMarginLeft, Metrics::ItemView_ItemMarginTop, Metrics::ItemView_ItemMarginRight, Metrics::ItemView_ItemMarginBottom);
    if (!option) {
        return margins;
    }

    const QFrame *frame = qobject_cast<const QFrame *>(option->widget);
    const QAbstractItemView *abstractItemView = qobject_cast<const QAbstractItemView *>(option->widget);

    const bool isFirst = option->index.row() == 0;
    const bool hasFrame = frame && frame->frameShape() == QFrame::StyledPanel;
    const bool reverse = option->direction == Qt::RightToLeft;

    if (isFirst) {
        margins.setTop(Metrics::ItemView_FirstItemTopMarginHeight);
    }

    // Nitrux frame has one extra white pixel
    if (hasFrame) {
        margins -= {1, isFirst ? 1 : 0, 1, 0};
    }

    // If a treeview wants to show the highlight separate for each column, always draw rounded borders
    // if (tree && !tree->allColumnsShowFocus()) {
    if (abstractItemView && abstractItemView->selectionBehavior() != QAbstractItemView::SelectRows) {
        return margins;
    }

    if ((reverse && option->viewItemPosition == QStyleOptionViewItem::End) || (!reverse && option->viewItemPosition == QStyleOptionViewItem::Beginning)
        || option->viewItemPosition == QStyleOptionViewItem::Middle) {
        margins.setRight(0);
    }
    if ((reverse && option->viewItemPosition == QStyleOptionViewItem::Beginning) || (!reverse && option->viewItemPosition == QStyleOptionViewItem::End)
        || option->viewItemPosition == QStyleOptionViewItem::Middle) {
        margins.setLeft(0);
    }

    return margins;
}

//______________________________________________________________________________
QPainterPath Helper::roundedPath(const QRectF &rect, Corners corners, qreal radius) const
{
    QPainterPath path;

    // simple cases
    if (corners == 0) {
        path.addRect(rect);
        return path;
    }

    if (corners == AllCorners) {
        path.addRoundedRect(rect, radius, radius);
        return path;
    }

    const QSizeF cornerSize(2 * radius, 2 * radius);

    // rotate counterclockwise
    // top left corner
    if (corners & CornerTopLeft) {
        path.moveTo(rect.topLeft() + QPointF(radius, 0));
        path.arcTo(QRectF(rect.topLeft(), cornerSize), 90, 90);

    } else {
        path.moveTo(rect.topLeft());
    }

    // bottom left corner
    if (corners & CornerBottomLeft) {
        path.lineTo(rect.bottomLeft() - QPointF(0, radius));
        path.arcTo(QRectF(rect.bottomLeft() - QPointF(0, 2 * radius), cornerSize), 180, 90);

    } else {
        path.lineTo(rect.bottomLeft());
    }

    // bottom right corner
    if (corners & CornerBottomRight) {
        path.lineTo(rect.bottomRight() - QPointF(radius, 0));
        path.arcTo(QRectF(rect.bottomRight() - QPointF(2 * radius, 2 * radius), cornerSize), 270, 90);

    } else {
        path.lineTo(rect.bottomRight());
    }

    // top right corner
    if (corners & CornerTopRight) {
        path.lineTo(rect.topRight() + QPointF(0, radius));
        path.arcTo(QRectF(rect.topRight() - QPointF(2 * radius, 0), cornerSize), 0, 90);

    } else {
        path.lineTo(rect.topRight());
    }

    path.closeSubpath();
    return path;
}

//____________________________________________________________________
bool Helper::hasAlphaChannel(const QWidget *widget) const
{
    return widget && widget->testAttribute(Qt::WA_TranslucentBackground);
}

//______________________________________________________________________________________

QPixmap Helper::coloredIcon(const QIcon &icon, const QPalette &palette, const QSize &size, qreal devicePixelRatio, QIcon::Mode mode, QIcon::State state)
{
    const QPalette activePalette = KIconLoader::global()->customPalette();
    const bool changePalette = activePalette != palette;
    if (changePalette) {
        KIconLoader::global()->setCustomPalette(palette);
    }
    const QPixmap pixmap = icon.pixmap(size, devicePixelRatio, mode, state);
    if (changePalette) {
        if (activePalette == QPalette()) {
            KIconLoader::global()->resetPalette();
        } else {
            KIconLoader::global()->setCustomPalette(activePalette);
        }
    }
    return pixmap;
}

bool Helper::shouldDrawToolsArea(const QWidget *widget) const
{
    if (!widget) {
        return false;
    }
    static bool isAuto = false;
    static QString borderSize;
    if (!_cachedAutoValid) {
        KConfigGroup kdecorationGroup(_kwinConfig->group(QStringLiteral("org.kde.kdecoration2")));
        isAuto = kdecorationGroup.readEntry("BorderSizeAuto", true);
        borderSize = kdecorationGroup.readEntry("BorderSize", "Normal");
        _cachedAutoValid = true;
    }
    if (isAuto) {
        auto window = widget->window();
        if (qobject_cast<const QDialog *>(widget)) {
            return true;
        }
        if (window) {
            auto handle = window->windowHandle();
            if (handle) {
                auto toolbar = qobject_cast<const QToolBar *>(widget);
                if (toolbar) {
                    if (toolbar->isFloating()) {
                        return false;
                    }
                }
                return true;
            }
        } else {
            return false;
        }
    }
    if (borderSize != "None" && borderSize != "NoSides") {
        return false;
    }
    return true;
}

Qt::Edges Helper::menuSeamlessEdges(const QWidget *widget)
{
    if (widget) {
        auto edges = widget->property(PropertyNames::menuSeamlessEdges).value<Qt::Edges>();
        // Fallback to older property
        if (edges == Qt::Edges() && widget->property(PropertyNames::isTopMenu).toBool()) {
            edges = Qt::TopEdge;
        }
        return edges;
    }
    return Qt::Edges();
}

qreal Helper::devicePixelRatio(QPainter *painter) const
{
    return painter->device() ? painter->device()->devicePixelRatioF() : qApp->devicePixelRatio();
}
}
