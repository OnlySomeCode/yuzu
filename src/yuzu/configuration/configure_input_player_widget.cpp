// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <QMenu>
#include <QPainter>
#include <QTimer>
#include "yuzu/configuration/configure_input_player_widget.h"

PlayerControlPreview::PlayerControlPreview(QWidget* parent) : QFrame(parent) {
    UpdateColors();
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&PlayerControlPreview::update));

    // refresh at 40hz
    timer->start(25);
}

PlayerControlPreview::~PlayerControlPreview() = default;

void PlayerControlPreview::SetPlayerInput(std::size_t index, const ButtonParam& buttons_param,
                                          const AnalogParam& analogs_param) {
    player_index = index;
    Settings::ButtonsRaw buttonss;
    Settings::AnalogsRaw analogs;
    std::transform(buttons_param.begin(), buttons_param.end(), buttonss.begin(),
                   [](const Common::ParamPackage& param) { return param.Serialize(); });
    std::transform(analogs_param.begin(), analogs_param.end(), analogs.begin(),
                   [](const Common::ParamPackage& param) { return param.Serialize(); });

    std::transform(buttonss.begin() + Settings::NativeButton::BUTTON_HID_BEGIN,
                   buttonss.begin() + Settings::NativeButton::BUTTON_NS_END, buttons.begin(),
                   Input::CreateDevice<Input::ButtonDevice>);
    std::transform(analogs.begin() + Settings::NativeAnalog::STICK_HID_BEGIN,
                   analogs.begin() + Settings::NativeAnalog::STICK_HID_END, sticks.begin(),
                   Input::CreateDevice<Input::AnalogDevice>);
    UpdateColors();
}
void PlayerControlPreview::SetPlayerInputRaw(std::size_t index, const Settings::ButtonsRaw buttons_,
                                             Settings::AnalogsRaw analogs_) {
    player_index = index;
    std::transform(buttons_.begin() + Settings::NativeButton::BUTTON_HID_BEGIN,
                   buttons_.begin() + Settings::NativeButton::BUTTON_NS_END, buttons.begin(),
                   Input::CreateDevice<Input::ButtonDevice>);
    std::transform(analogs_.begin() + Settings::NativeAnalog::STICK_HID_BEGIN,
                   analogs_.begin() + Settings::NativeAnalog::STICK_HID_END, sticks.begin(),
                   Input::CreateDevice<Input::AnalogDevice>);
    UpdateColors();
}

PlayerControlPreview::LedPattern PlayerControlPreview::GetColorPattern(std::size_t index,
                                                                       bool player_on) {
    if (!player_on) {
        return {0, 0, 0, 0};
    }

    switch (index) {
    case 0:
        return {1, 0, 0, 0};
    case 1:
        return {1, 1, 0, 0};
    case 2:
        return {1, 1, 1, 0};
    case 3:
        return {1, 1, 1, 1};
    case 4:
        return {1, 0, 0, 1};
    case 5:
        return {1, 0, 1, 0};
    case 6:
        return {1, 0, 1, 1};
    case 7:
        return {0, 1, 1, 0};
    default:
        return {0, 0, 0, 0};
    }
}

void PlayerControlPreview::SetConnectedStatus(bool checked) {
    LedPattern led_pattern = GetColorPattern(player_index, checked);

    led_color[0] = led_pattern.position1 ? colors.led_on : colors.led_off;
    led_color[1] = led_pattern.position2 ? colors.led_on : colors.led_off;
    led_color[2] = led_pattern.position3 ? colors.led_on : colors.led_off;
    led_color[3] = led_pattern.position4 ? colors.led_on : colors.led_off;
}

void PlayerControlPreview::SetControllerType(const Settings::ControllerType type) {
    controller_type = type;
    UpdateColors();
}

void PlayerControlPreview::BeginMappingButton(std::size_t index) {
    button_mapping_index = index;
    mapping_active = true;
}

void PlayerControlPreview::BeginMappingAnalog(std::size_t index) {
    button_mapping_index = Settings::NativeButton::LStick + index;
    analog_mapping_index = index;
    mapping_active = true;
}

void PlayerControlPreview::EndMapping() {
    button_mapping_index = Settings::NativeButton::BUTTON_NS_END;
    analog_mapping_index = Settings::NativeAnalog::NumAnalogs;
    mapping_active = false;
    blink_counter = 0;
}

void PlayerControlPreview::UpdateColors() {
    if (QIcon::themeName().contains(QStringLiteral("dark")) ||
        QIcon::themeName().contains(QStringLiteral("midnight"))) {
        colors.primary = QColor(204, 204, 204);
        colors.button = QColor(35, 38, 41);
        colors.button2 = QColor(26, 27, 30);
        colors.slider_arrow = QColor(14, 15, 18);
        colors.font2 = QColor(255, 255, 255);
        colors.indicator = QColor(170, 238, 255);
        colors.deadzone = QColor(204, 136, 136);
        colors.slider_button = colors.button;
    }

    if (QIcon::themeName().contains(QStringLiteral("dark"))) {
        colors.outline = QColor(160, 160, 160);
    } else if (QIcon::themeName().contains(QStringLiteral("midnight"))) {
        colors.outline = QColor(145, 145, 145);
    } else {
        colors.outline = QColor(0, 0, 0);
        colors.primary = QColor(225, 225, 225);
        colors.button = QColor(109, 111, 114);
        colors.button2 = QColor(109, 111, 114);
        colors.button2 = QColor(77, 80, 84);
        colors.slider_arrow = QColor(65, 68, 73);
        colors.font2 = QColor(0, 0, 0);
        colors.indicator = QColor(0, 0, 200);
        colors.deadzone = QColor(170, 0, 0);
        colors.slider_button = QColor(153, 149, 149);
    }

    // Constant colors
    colors.highlight = QColor(170, 0, 0);
    colors.highlight2 = QColor(119, 0, 0);
    colors.slider = QColor(103, 106, 110);
    colors.transparent = QColor(0, 0, 0, 0);
    colors.font = QColor(255, 255, 255);
    colors.led_on = QColor(255, 255, 0);
    colors.led_off = QColor(170, 238, 255);

    colors.left = colors.primary;
    colors.right = colors.primary;
    // Possible alternative to set colors from settings
    // colors.left = QColor(Settings::values.players.GetValue()[player_index].body_color_left);
    // colors.right = QColor(Settings::values.players.GetValue()[player_index].body_color_right);
}

void PlayerControlPreview::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPointF center = rect().center();

    const auto& button_state = buttons;
    for (std::size_t index = 0; index < button_values.size(); ++index) {
        bool value = false;
        if (index < Settings::NativeButton::BUTTON_NS_END) {
            value = button_state[index]->GetStatus();
        }
        bool blink = mapping_active && index == button_mapping_index;
        if (analog_mapping_index == Settings::NativeAnalog::NUM_STICKS_HID) {
            blink &= blink_counter > 12;
        }
        button_values[index] = value || blink;
    }

    const auto& analog_state = sticks;
    for (std::size_t index = 0; index < axis_values.size(); ++index) {
        const auto [stick_x_f, stick_y_f] = analog_state[index]->GetStatus();
        const auto [stick_x_rf, stick_y_rf] = analog_state[index]->GetRawStatus();
        axis_values[index].properties = analog_state[index]->GetAnalogProperties();
        axis_values[index].value = QPointF(stick_x_f, -stick_y_f);
        axis_values[index].raw_value = QPointF(stick_x_rf, -stick_y_rf);

        const bool blink_analog = mapping_active && index == analog_mapping_index;
        if (blink_analog) {
            axis_values[index].value =
                QPointF(blink_counter < 12 ? -blink_counter / 12.0f : 0,
                        blink_counter > 12 ? -(blink_counter - 12) / 12.0f : 0);
        }
    }
    switch (controller_type) {
    case Settings::ControllerType::Handheld:
        DrawHandheldController(p, center);
        break;
    case Settings::ControllerType::DualJoyconDetached:
        DrawDualController(p, center);
        break;
    case Settings::ControllerType::LeftJoycon:
        DrawLeftController(p, center);
        break;
    case Settings::ControllerType::RightJoycon:
        DrawRightController(p, center);
        break;
    case Settings::ControllerType::ProController:
    default:
        DrawProController(p, center);
        break;
    }
    if (mapping_active) {
        blink_counter = (blink_counter + 1) % 24;
    }
}

void PlayerControlPreview::DrawLeftController(QPainter& p, const QPointF center) {
    {
        using namespace Settings::NativeButton;

        // Sideview left joystick
        DrawJoystickSideview(p, center + QPoint(142, -69),
                             -axis_values[Settings::NativeAnalog::LStick].value.y(), 1.15f,
                             button_values[LStick]);

        // Topview D-pad buttons
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(-163, -21), button_values[DLeft], 11, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(-117, -21), button_values[DRight], 11, 5, Direction::Up);

        // Topview left joystick
        DrawJoystickSideview(p, center + QPointF(-140.5f, -28),
                             -axis_values[Settings::NativeAnalog::LStick].value.x() + 15.0f, 1.15f,
                             button_values[LStick]);

        // Topview minus button
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(-111, -22), button_values[Minus], 8, 4, Direction::Up,
                        1);

        // Left trigger
        DrawLeftTriggers(p, center, button_values[L]);
        DrawRoundButton(p, center + QPoint(151, -146), button_values[L], 8, 4, Direction::Down);
        DrawLeftZTriggers(p, center, button_values[ZL]);

        // Sideview D-pad buttons
        DrawRoundButton(p, center + QPoint(135, 14), button_values[DLeft], 5, 11, Direction::Right);
        DrawRoundButton(p, center + QPoint(135, 36), button_values[DDown], 5, 11, Direction::Right);
        DrawRoundButton(p, center + QPoint(135, -10), button_values[DUp], 5, 11, Direction::Right);
        DrawRoundButton(p, center + QPoint(135, 14), button_values[DRight], 5, 11,
                        Direction::Right);
        DrawRoundButton(p, center + QPoint(135, 71), button_values[Screenshot], 3, 8,
                        Direction::Right, 1);

        // Sideview minus button
        DrawRoundButton(p, center + QPoint(135, -118), button_values[Minus], 4, 2.66f,
                        Direction::Right, 1);

        // Sideview SL and SR buttons
        button_color = colors.slider_button;
        DrawRoundButton(p, center + QPoint(59, 52), button_values[SR], 5, 12, Direction::Left);
        DrawRoundButton(p, center + QPoint(59, -69), button_values[SL], 5, 12, Direction::Left);

        DrawLeftBody(p, center);

        // Left trigger top view
        DrawLeftTriggersTopView(p, center, button_values[L]);
        DrawLeftZTriggersTopView(p, center, button_values[ZL]);
    }

    { // Draw joysticks
        using namespace Settings::NativeAnalog;
        DrawJoystick(p, center + QPointF(9, -69) + (axis_values[LStick].value * 8), 1.8f,
                     button_values[Settings::NativeButton::LStick]);
        DrawRawJoystick(p, center + QPointF(-140, 100), axis_values[LStick].raw_value,
                        axis_values[LStick].properties);
    }

    using namespace Settings::NativeButton;

    // D-pad constants
    const QPointF dpad_center = center + QPoint(9, 14);
    constexpr int dpad_distance = 23;
    constexpr int dpad_radius = 11;
    constexpr float dpad_arrow_size = 1.2f;

    // D-pad buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, dpad_center + QPoint(dpad_distance, 0), button_values[DRight], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, dpad_distance), button_values[DDown], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, -dpad_distance), button_values[DUp], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(-dpad_distance, 0), button_values[DLeft], dpad_radius);

    // D-pad arrows
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, dpad_center + QPoint(dpad_distance, 0), Direction::Right, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, dpad_distance), Direction::Down, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, -dpad_distance), Direction::Up, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(-dpad_distance, 0), Direction::Left, dpad_arrow_size);

    // SR and SL buttons
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawRoundButton(p, center + QPoint(155, 52), button_values[SR], 5.2f, 12, Direction::None, 4);
    DrawRoundButton(p, center + QPoint(155, -69), button_values[SL], 5.2f, 12, Direction::None, 4);

    // SR and SL text
    SetTextFont(p, 5.7f);
    p.setPen(colors.outline);
    p.rotate(90);
    p.drawText(QPointF(center.y() - 5, -center.x() + 3) + QPointF(52, -155), tr("SR"));
    p.drawText(QPointF(center.y() - 5, -center.x() + 3) + QPointF(-69, -155), tr("SL"));
    p.rotate(-90);

    // Minus button
    button_color = colors.button;
    DrawMinusButton(p, center + QPoint(39, -118), button_values[Minus], 16);

    // Screenshot button
    DrawRoundButton(p, center + QPoint(26, 71), button_values[Screenshot], 8, 8);
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawCircle(p, center + QPoint(26, 71), 5);
}

void PlayerControlPreview::DrawRightController(QPainter& p, const QPointF center) {
    {
        using namespace Settings::NativeButton;

        // Sideview right joystick
        DrawJoystickSideview(p, center + QPoint(173 - 315, 11),
                             axis_values[Settings::NativeAnalog::RStick].value.y() + 10.0f, 1.15f,
                             button_values[Settings::NativeButton::RStick]);

        // Topview face buttons
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(163, -21), button_values[A], 11, 5, Direction::Up);
        DrawRoundButton(p, center + QPoint(117, -21), button_values[Y], 11, 5, Direction::Up);

        // Topview right joystick
        DrawJoystickSideview(p, center + QPointF(140, -28),
                             -axis_values[Settings::NativeAnalog::RStick].value.x() + 15.0f, 1.15f,
                             button_values[RStick]);

        // Topview plus button
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(111, -22), button_values[Plus], 8, 4, Direction::Up, 1);
        DrawRoundButton(p, center + QPoint(111, -22), button_values[Plus], 2.66f, 4, Direction::Up,
                        1);

        // Right trigger
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRightTriggers(p, center, button_values[R]);
        DrawRoundButton(p, center + QPoint(-151, -146), button_values[R], 8, 4, Direction::Down);
        DrawRightZTriggers(p, center, button_values[ZR]);

        // Sideview face buttons
        DrawRoundButton(p, center + QPoint(-135, -73), button_values[A], 5, 11, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -50), button_values[B], 5, 11, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -95), button_values[X], 5, 11, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -73), button_values[Y], 5, 11, Direction::Left);

        // Sideview home and plus button
        DrawRoundButton(p, center + QPoint(-135, 66), button_values[Home], 3, 12, Direction::Left);
        DrawRoundButton(p, center + QPoint(-135, -118), button_values[Plus], 4, 8, Direction::Left,
                        1);
        DrawRoundButton(p, center + QPoint(-135, -118), button_values[Plus], 4, 2.66f,
                        Direction::Left, 1);

        // Sideview SL and SR buttons
        button_color = colors.slider_button;
        DrawRoundButton(p, center + QPoint(-59, 52), button_values[SL], 5, 11, Direction::Right);
        DrawRoundButton(p, center + QPoint(-59, -69), button_values[SR], 5, 11, Direction::Right);

        DrawRightBody(p, center);

        // Right trigger top view
        DrawRightTriggersTopView(p, center, button_values[R]);
        DrawRightZTriggersTopView(p, center, button_values[ZR]);
    }

    { // Draw joysticks
        using namespace Settings::NativeAnalog;
        DrawJoystick(p, center + QPointF(-9, 11) + (axis_values[RStick].value * 8), 1.8f,
                     button_values[Settings::NativeButton::RStick]);
        DrawRawJoystick(p, center + QPointF(140, 100), axis_values[RStick].raw_value,
                        axis_values[RStick].properties);
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    const QPointF face_center = center + QPoint(-9, -73);
    constexpr int face_distance = 23;
    constexpr int face_radius = 11;
    constexpr float text_size = 1.1f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, face_center + QPoint(face_distance, 0), button_values[A], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, face_distance), button_values[B], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, -face_distance), button_values[X], face_radius);
    DrawCircleButton(p, face_center + QPoint(-face_distance, 0), button_values[Y], face_radius);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, face_center + QPoint(face_distance, 0), Symbol::A, text_size);
    DrawSymbol(p, face_center + QPoint(0, face_distance), Symbol::B, text_size);
    DrawSymbol(p, face_center + QPoint(0, -face_distance), Symbol::X, text_size);
    DrawSymbol(p, face_center + QPoint(-face_distance, 1), Symbol::Y, text_size);

    // SR and SL buttons
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawRoundButton(p, center + QPoint(-155, 52), button_values[SL], 5, 12, Direction::None, 4.0f);
    DrawRoundButton(p, center + QPoint(-155, -69), button_values[SR], 5, 12, Direction::None, 4.0f);

    // SR and SL text
    SetTextFont(p, 5.7f);
    p.setPen(colors.outline);
    p.rotate(-90);
    p.drawText(QPointF(-center.y() - 5, center.x() + 3) + QPointF(-52, -155), tr("SL"));
    p.drawText(QPointF(-center.y() - 5, center.x() + 3) + QPointF(69, -155), tr("SR"));
    p.rotate(90);

    // Plus Button
    DrawPlusButton(p, center + QPoint(-40, -118), button_values[Plus], 16);

    // Home Button
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawCircleButton(p, center + QPoint(-26, 66), button_values[Home], 12);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(-26, 66), button_values[Home], 9);
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPoint(-26, 66), Symbol::House, 5);
}

void PlayerControlPreview::DrawDualController(QPainter& p, const QPointF center) {
    {
        using namespace Settings::NativeButton;

        // Sideview joystick
        DrawJoystickSideview(p, center + QPoint(-174, -65),
                             -axis_values[Settings::NativeAnalog::LStick].value.y(), 1.0f,
                             button_values[LStick]);
        DrawJoystickSideview(p, center + QPoint(174, 12),
                             axis_values[Settings::NativeAnalog::RStick].value.y() + 10.0f, 1.0f,
                             button_values[RStick]);

        // Left/Right trigger
        DrawDualTriggers(p, center, button_values[L], button_values[R]);
        DrawDualZTriggers(p, center, button_values[ZL], button_values[ZR]);

        // sideview Left and Right trigger
        p.setPen(colors.outline);
        button_color = colors.button;
        DrawRoundButton(p, center + QPoint(-166, -131), button_values[L], 7, 4, Direction::Down);
        DrawRoundButton(p, center + QPoint(166, -131), button_values[R], 7, 4, Direction::Down);

        // Sideview face buttons
        DrawRoundButton(p, center + QPoint(180, -65), button_values[A], 5, 10, Direction::Left);
        DrawRoundButton(p, center + QPoint(180, -45), button_values[B], 5, 10, Direction::Left);
        DrawRoundButton(p, center + QPoint(180, -85), button_values[X], 5, 10, Direction::Left);
        DrawRoundButton(p, center + QPoint(180, -65), button_values[Y], 5, 10, Direction::Left);

        // Sideview D-pad buttons
        DrawRoundButton(p, center + QPoint(-180, 12), button_values[DLeft], 5, 10,
                        Direction::Right);
        DrawRoundButton(p, center + QPoint(-180, 33), button_values[DDown], 5, 10,
                        Direction::Right);
        DrawRoundButton(p, center + QPoint(-180, -8), button_values[DUp], 5, 10, Direction::Right);
        DrawRoundButton(p, center + QPoint(-180, 12), button_values[DRight], 5, 10,
                        Direction::Right);

        // Sideview home and plus button
        DrawRoundButton(p, center + QPoint(180, 60), button_values[Home], 3, 11, Direction::Left);
        DrawRoundButton(p, center + QPoint(180, -106), button_values[Plus], 4, 7, Direction::Left,
                        1);
        DrawRoundButton(p, center + QPoint(180, -106), button_values[Plus], 4, 2.33f,
                        Direction::Left, 1);

        // Sideview screenshot and minus button
        DrawRoundButton(p, center + QPoint(-180, 63), button_values[Screenshot], 3, 8,
                        Direction::Right, 1);
        DrawRoundButton(p, center + QPoint(-180, -106), button_values[Minus], 4, 2.66f,
                        Direction::Right, 1);
    }

    DrawDualBody(p, center);

    { // Draw joysticks
        using namespace Settings::NativeAnalog;
        DrawJoystick(p, center + QPointF(-65, -65) + (axis_values[LStick].value * 7), 1.62f,
                     button_values[Settings::NativeButton::LStick]);
        DrawJoystick(p, center + QPointF(65, 12) + (axis_values[RStick].value * 7), 1.62f,
                     button_values[Settings::NativeButton::RStick]);
        DrawRawJoystick(p, rect().bottomLeft() + QPointF(45, -45), axis_values[LStick].raw_value,
                        axis_values[LStick].properties);
        DrawRawJoystick(p, rect().bottomRight() + QPointF(-45, -45), axis_values[RStick].raw_value,
                        axis_values[RStick].properties);
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    const QPointF face_center = center + QPoint(65, -65);
    constexpr int face_distance = 20;
    constexpr int face_radius = 10;
    constexpr float text_size = 1.0f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, face_center + QPoint(face_distance, 0), button_values[A], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, face_distance), button_values[B], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, -face_distance), button_values[X], face_radius);
    DrawCircleButton(p, face_center + QPoint(-face_distance, 0), button_values[Y], face_radius);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, face_center + QPoint(face_distance, 0), Symbol::A, text_size);
    DrawSymbol(p, face_center + QPoint(0, face_distance), Symbol::B, text_size);
    DrawSymbol(p, face_center + QPoint(0, -face_distance), Symbol::X, text_size);
    DrawSymbol(p, face_center + QPoint(-face_distance, 1), Symbol::Y, text_size);

    // D-pad constants
    const QPointF dpad_center = center + QPoint(-65, 12);
    constexpr int dpad_distance = 20;
    constexpr int dpad_radius = 10;
    constexpr float dpad_arrow_size = 1.1f;

    // D-pad buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, dpad_center + QPoint(dpad_distance, 0), button_values[DRight], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, dpad_distance), button_values[DDown], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, -dpad_distance), button_values[DUp], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(-dpad_distance, 0), button_values[DLeft], dpad_radius);

    // D-pad arrows
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, dpad_center + QPoint(dpad_distance, 0), Direction::Right, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, dpad_distance), Direction::Down, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, -dpad_distance), Direction::Up, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(-dpad_distance, 0), Direction::Left, dpad_arrow_size);

    // Minus and Plus button
    button_color = colors.button;
    DrawMinusButton(p, center + QPoint(-39, -106), button_values[Minus], 14);
    DrawPlusButton(p, center + QPoint(39, -106), button_values[Plus], 14);

    // Screenshot button
    p.setPen(colors.outline);
    DrawRoundButton(p, center + QPoint(-52, 63), button_values[Screenshot], 8, 8);
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawCircle(p, center + QPoint(-52, 63), 5);

    // Home Button
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawCircleButton(p, center + QPoint(50, 60), button_values[Home], 11);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(50, 60), button_values[Home], 8.5f);
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPoint(50, 60), Symbol::House, 4.2f);
}

void PlayerControlPreview::DrawHandheldController(QPainter& p, const QPointF center) {
    DrawHandheldTriggers(p, center, button_values[Settings::NativeButton::L],
                         button_values[Settings::NativeButton::R]);
    DrawHandheldBody(p, center);
    { // Draw joysticks
        using namespace Settings::NativeAnalog;
        DrawJoystick(p, center + QPointF(-171, -41) + (axis_values[LStick].value * 4), 1.0f,
                     button_values[Settings::NativeButton::LStick]);
        DrawJoystick(p, center + QPointF(171, 8) + (axis_values[RStick].value * 4), 1.0f,
                     button_values[Settings::NativeButton::RStick]);
        DrawRawJoystick(p, center + QPointF(-50, 0), axis_values[LStick].raw_value,
                        axis_values[LStick].properties);
        DrawRawJoystick(p, center + QPointF(50, 0), axis_values[RStick].raw_value,
                        axis_values[RStick].properties);
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    const QPointF face_center = center + QPoint(171, -41);
    constexpr float face_distance = 12.8;
    constexpr float face_radius = 6.4f;
    constexpr float text_size = 0.6f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, face_center + QPoint(face_distance, 0), button_values[A], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, face_distance), button_values[B], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, -face_distance), button_values[X], face_radius);
    DrawCircleButton(p, face_center + QPoint(-face_distance, 0), button_values[Y], face_radius);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, face_center + QPoint(face_distance, 0), Symbol::A, text_size);
    DrawSymbol(p, face_center + QPoint(0, face_distance), Symbol::B, text_size);
    DrawSymbol(p, face_center + QPoint(0, -face_distance), Symbol::X, text_size);
    DrawSymbol(p, face_center + QPoint(-face_distance, 1), Symbol::Y, text_size);

    // D-pad constants
    const QPointF dpad_center = center + QPoint(-171, 8);
    constexpr float dpad_distance = 12.8;
    constexpr float dpad_radius = 6.4f;
    constexpr float dpad_arrow_size = 0.68f;

    // D-pad buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, dpad_center + QPoint(dpad_distance, 0), button_values[DRight], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, dpad_distance), button_values[DDown], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(0, -dpad_distance), button_values[DUp], dpad_radius);
    DrawCircleButton(p, dpad_center + QPoint(-dpad_distance, 0), button_values[DLeft], dpad_radius);

    // D-pad arrows
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, dpad_center + QPoint(dpad_distance, 0), Direction::Right, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, dpad_distance), Direction::Down, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(0, -dpad_distance), Direction::Up, dpad_arrow_size);
    DrawArrow(p, dpad_center + QPoint(-dpad_distance, 0), Direction::Left, dpad_arrow_size);

    // ZL and ZR buttons
    p.setPen(colors.outline);
    DrawTriggerButton(p, center + QPoint(-210, -130), Direction::Left, button_values[ZL]);
    DrawTriggerButton(p, center + QPoint(210, -130), Direction::Right, button_values[ZR]);
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, center + QPoint(-210, -130), Symbol::ZL, 1.5f);
    DrawSymbol(p, center + QPoint(210, -130), Symbol::ZR, 1.5f);

    // Minus and Plus button
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawMinusButton(p, center + QPoint(-155, -67), button_values[Minus], 8);
    DrawPlusButton(p, center + QPoint(155, -67), button_values[Plus], 8);

    // Screenshot button
    p.setPen(colors.outline);
    DrawRoundButton(p, center + QPoint(-162, 39), button_values[Screenshot], 5, 5);
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawCircle(p, center + QPoint(-162, 39), 3);

    // Home Button
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawCircleButton(p, center + QPoint(161, 37), button_values[Home], 7);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(161, 37), button_values[Home], 5);
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPoint(161, 37), Symbol::House, 2.75f);
}

void PlayerControlPreview::DrawProController(QPainter& p, const QPointF center) {
    DrawProTriggers(p, center, button_values[Settings::NativeButton::L],
                    button_values[Settings::NativeButton::R]);
    DrawProBody(p, center);
    { // Draw joysticks
        using namespace Settings::NativeAnalog;
        DrawProJoystick(p, center + QPointF(-111, -55) + (axis_values[LStick].value * 11),
                        button_values[Settings::NativeButton::LStick]);
        DrawProJoystick(p, center + QPointF(51, 0) + (axis_values[RStick].value * 11),
                        button_values[Settings::NativeButton::RStick]);
        DrawRawJoystick(p, center + QPointF(-50, 105), axis_values[LStick].raw_value,
                        axis_values[LStick].properties);
        DrawRawJoystick(p, center + QPointF(50, 105), axis_values[RStick].raw_value,
                        axis_values[RStick].properties);
    }

    using namespace Settings::NativeButton;

    // Face buttons constants
    const QPointF face_center = center + QPoint(105, -56);
    constexpr int face_distance = 31;
    constexpr int face_radius = 15;
    constexpr float text_size = 1.5f;

    // Face buttons
    p.setPen(colors.outline);
    button_color = colors.button;
    DrawCircleButton(p, face_center + QPoint(face_distance, 0), button_values[A], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, face_distance), button_values[B], face_radius);
    DrawCircleButton(p, face_center + QPoint(0, -face_distance), button_values[X], face_radius);
    DrawCircleButton(p, face_center + QPoint(-face_distance, 0), button_values[Y], face_radius);

    // Face buttons text
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, face_center + QPoint(face_distance, 0), Symbol::A, text_size);
    DrawSymbol(p, face_center + QPoint(0, face_distance), Symbol::B, text_size);
    DrawSymbol(p, face_center + QPoint(0, -face_distance), Symbol::X, text_size);
    DrawSymbol(p, face_center + QPoint(-face_distance, 1), Symbol::Y, text_size);

    // D-pad buttons
    const QPointF dpad_postion = center + QPoint(-61, 0);
    DrawArrowButton(p, dpad_postion, Direction::Up, button_values[DUp]);
    DrawArrowButton(p, dpad_postion, Direction::Left, button_values[DLeft]);
    DrawArrowButton(p, dpad_postion, Direction::Right, button_values[DRight]);
    DrawArrowButton(p, dpad_postion, Direction::Down, button_values[DDown]);
    DrawArrowButtonOutline(p, dpad_postion);

    // ZL and ZR buttons
    p.setPen(colors.outline);
    DrawTriggerButton(p, center + QPoint(-210, -130), Direction::Left, button_values[ZL]);
    DrawTriggerButton(p, center + QPoint(210, -130), Direction::Right, button_values[ZR]);
    p.setPen(colors.transparent);
    p.setBrush(colors.font);
    DrawSymbol(p, center + QPoint(-210, -130), Symbol::ZL, 1.5f);
    DrawSymbol(p, center + QPoint(210, -130), Symbol::ZR, 1.5f);

    // Minus and Plus buttons
    p.setPen(colors.outline);
    DrawCircleButton(p, center + QPoint(-50, -86), button_values[Minus], 9);
    DrawCircleButton(p, center + QPoint(50, -86), button_values[Plus], 9);

    // Minus and Plus symbols
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawRectangle(p, center + QPoint(-50, -86), 9, 1.5f);
    DrawRectangle(p, center + QPoint(50, -86), 9, 1.5f);
    DrawRectangle(p, center + QPoint(50, -86), 1.5f, 9);

    // Screenshot button
    p.setPen(colors.outline);
    DrawRoundButton(p, center + QPoint(-29, -56), button_values[Screenshot], 7, 7);
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawCircle(p, center + QPoint(-29, -56), 4.5f);

    // Home Button
    p.setPen(colors.outline);
    button_color = colors.slider_button;
    DrawCircleButton(p, center + QPoint(29, -56), button_values[Home], 10.0f);
    button_color = colors.button;
    DrawCircleButton(p, center + QPoint(29, -56), button_values[Home], 7.1f);
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPoint(29, -56), Symbol::House, 3.9f);
}

constexpr std::array<float, 13 * 2> symbol_a = {
    -1.085f, -5.2f,   1.085f, -5.2f,   5.085f, 5.0f,    2.785f,  5.0f,  1.785f,
    2.65f,   -1.785f, 2.65f,  -2.785f, 5.0f,   -5.085f, 5.0f,    -1.4f, 1.0f,
    0.0f,    -2.8f,   1.4f,   1.0f,    -1.4f,  1.0f,    -5.085f, 5.0f,
};
constexpr std::array<float, 134 * 2> symbol_b = {
    -4.0f, 0.0f,  -4.0f, 0.0f,  -4.0f, -0.1f, -3.8f, -5.1f, 1.8f,  -5.0f, 2.3f,  -4.9f, 2.6f,
    -4.8f, 2.8f,  -4.7f, 2.9f,  -4.6f, 3.1f,  -4.5f, 3.2f,  -4.4f, 3.4f,  -4.3f, 3.4f,  -4.2f,
    3.5f,  -4.1f, 3.7f,  -4.0f, 3.7f,  -3.9f, 3.8f,  -3.8f, 3.8f,  -3.7f, 3.9f,  -3.6f, 3.9f,
    -3.5f, 4.0f,  -3.4f, 4.0f,  -3.3f, 4.1f,  -3.1f, 4.1f,  -3.0f, 4.0f,  -2.0f, 4.0f,  -1.9f,
    3.9f,  -1.7f, 3.9f,  -1.6f, 3.8f,  -1.5f, 3.8f,  -1.4f, 3.7f,  -1.3f, 3.7f,  -1.2f, 3.6f,
    -1.1f, 3.6f,  -1.0f, 3.5f,  -0.9f, 3.3f,  -0.8f, 3.3f,  -0.7f, 3.2f,  -0.6f, 3.0f,  -0.5f,
    2.9f,  -0.4f, 2.7f,  -0.3f, 2.9f,  -0.2f, 3.2f,  -0.1f, 3.3f,  0.0f,  3.5f,  0.1f,  3.6f,
    0.2f,  3.8f,  0.3f,  3.9f,  0.4f,  4.0f,  0.6f,  4.1f,  0.7f,  4.3f,  0.8f,  4.3f,  0.9f,
    4.4f,  1.0f,  4.4f,  1.1f,  4.5f,  1.3f,  4.5f,  1.4f,  4.6f,  1.6f,  4.6f,  1.7f,  4.5f,
    2.8f,  4.5f,  2.9f,  4.4f,  3.1f,  4.4f,  3.2f,  4.3f,  3.4f,  4.3f,  3.5f,  4.2f,  3.6f,
    4.2f,  3.7f,  4.1f,  3.8f,  4.1f,  3.9f,  4.0f,  4.0f,  3.9f,  4.2f,  3.8f,  4.3f,  3.6f,
    4.4f,  3.6f,  4.5f,  3.4f,  4.6f,  3.3f,  4.7f,  3.1f,  4.8f,  2.8f,  4.9f,  2.6f,  5.0f,
    2.1f,  5.1f,  -4.0f, 5.0f,  -4.0f, 4.9f,

    -4.0f, 0.0f,  1.1f,  3.4f,  1.1f,  3.4f,  1.5f,  3.3f,  1.8f,  3.2f,  2.0f,  3.1f,  2.1f,
    3.0f,  2.3f,  2.9f,  2.3f,  2.8f,  2.4f,  2.7f,  2.4f,  2.6f,  2.5f,  2.3f,  2.5f,  2.2f,
    2.4f,  1.7f,  2.4f,  1.6f,  2.3f,  1.4f,  2.3f,  1.3f,  2.2f,  1.2f,  2.2f,  1.1f,  2.1f,
    1.0f,  1.9f,  0.9f,  1.6f,  0.8f,  1.4f,  0.7f,  -1.9f, 0.6f,  -1.9f, 0.7f,  -1.8f, 3.4f,
    1.1f,  3.4f,  -4.0f, 0.0f,

    0.3f,  -1.1f, 0.3f,  -1.1f, 1.3f,  -1.2f, 1.5f,  -1.3f, 1.8f,  -1.4f, 1.8f,  -1.5f, 1.9f,
    -1.6f, 2.0f,  -1.8f, 2.0f,  -1.9f, 2.1f,  -2.0f, 2.1f,  -2.1f, 2.0f,  -2.7f, 2.0f,  -2.8f,
    1.9f,  -2.9f, 1.9f,  -3.0f, 1.8f,  -3.1f, 1.6f,  -3.2f, 1.6f,  -3.3f, 1.3f,  -3.4f, -1.9f,
    -3.3f, -1.9f, -3.2f, -1.8f, -1.0f, 0.2f,  -1.1f, 0.3f,  -1.1f, -4.0f, 0.0f,
};

constexpr std::array<float, 9 * 2> symbol_y = {
    -4.79f, -4.9f, -2.44f, -4.9f, 0.0f,  -0.9f,  2.44f, -4.9f,  4.79f,
    -4.9f,  1.05f, 1.0f,   1.05f, 5.31f, -1.05f, 5.31f, -1.05f, 1.0f,

};

constexpr std::array<float, 12 * 2> symbol_x = {
    -4.4f, -5.0f, -2.0f, -5.0f, 0.0f, -1.7f, 2.0f,  -5.0f, 4.4f,  -5.0f, 1.2f,  0.0f,
    4.4f,  5.0f,  2.0f,  5.0f,  0.0f, 1.7f,  -2.0f, 5.0f,  -4.4f, 5.0f,  -1.2f, 0.0f,

};

constexpr std::array<float, 18 * 2> symbol_zl = {
    -2.6f, -2.13f, -5.6f, -2.13f, -5.6f, -3.23f, -0.8f, -3.23f, -0.8f, -2.13f, -4.4f, 2.12f,
    -0.7f, 2.12f,  -0.7f, 3.22f,  -6.0f, 3.22f,  -6.0f, 2.12f,  2.4f,  -3.23f, 2.4f,  2.1f,
    5.43f, 2.1f,   5.43f, 3.22f,  0.98f, 3.22f,  0.98f, -3.23f, 2.4f,  -3.23f, -6.0f, 2.12f,
};

constexpr std::array<float, 110 * 2> symbol_zr = {
    -2.6f, -2.13f, -5.6f, -2.13f, -5.6f, -3.23f, -0.8f, -3.23f, -0.8f, -2.13f, -4.4f, 2.12f, -0.7f,
    2.12f, -0.7f,  3.22f, -6.0f,  3.22f, -6.0f,  2.12f,

    1.0f,  0.0f,   1.0f,  -0.1f,  1.1f,  -3.3f,  4.3f,  -3.2f,  5.1f,  -3.1f,  5.4f,  -3.0f, 5.6f,
    -2.9f, 5.7f,   -2.8f, 5.9f,   -2.7f, 5.9f,   -2.6f, 6.0f,   -2.5f, 6.1f,   -2.3f, 6.2f,  -2.2f,
    6.2f,  -2.1f,  6.3f,  -2.0f,  6.3f,  -1.9f,  6.2f,  -0.8f,  6.2f,  -0.7f,  6.1f,  -0.6f, 6.1f,
    -0.5f, 6.0f,   -0.4f, 6.0f,   -0.3f, 5.9f,   -0.2f, 5.7f,   -0.1f, 5.7f,   0.0f,  5.6f,  0.1f,
    5.4f,  0.2f,   5.1f,  0.3f,   4.7f,  0.4f,   4.7f,  0.5f,   4.9f,  0.6f,   5.0f,  0.7f,  5.2f,
    0.8f,  5.2f,   0.9f,  5.3f,   1.0f,  5.5f,   1.1f,  5.5f,   1.2f,  5.6f,   1.3f,  5.7f,  1.5f,
    5.8f,  1.6f,   5.9f,  1.8f,   6.0f,  1.9f,   6.1f,  2.1f,   6.2f,  2.2f,   6.2f,  2.3f,  6.3f,
    2.4f,  6.4f,   2.6f,  6.5f,   2.7f,  6.6f,   2.9f,  6.7f,   3.0f,  6.7f,   3.1f,  6.8f,  3.2f,
    6.8f,  3.3f,   5.3f,  3.2f,   5.2f,  3.1f,   5.2f,  3.0f,   5.1f,  2.9f,   5.0f,  2.7f,  4.9f,
    2.6f,  4.8f,   2.4f,  4.7f,   2.3f,  4.6f,   2.1f,  4.5f,   2.0f,  4.4f,   1.8f,  4.3f,  1.7f,
    4.1f,  1.4f,   4.0f,  1.3f,   3.9f,  1.1f,   3.8f,  1.0f,   3.6f,  0.9f,   3.6f,  0.8f,  3.5f,
    0.7f,  3.3f,   0.6f,  2.9f,   0.5f,  2.3f,   0.6f,  2.3f,   0.7f,  2.2f,   3.3f,  1.0f,  3.2f,
    1.0f,  3.1f,   1.0f,  0.0f,

    4.2f,  -0.5f,  4.2f,  -0.5f,  4.4f,  -0.6f,  4.7f,  -0.7f,  4.8f,  -0.8f,  4.9f,  -1.0f, 5.0f,
    -1.1f, 5.0f,   -1.2f, 4.9f,   -1.7f, 4.9f,   -1.8f, 4.8f,   -1.9f, 4.8f,   -2.0f, 4.6f,  -2.1f,
    4.3f,  -2.2f,  2.3f,  -2.1f,  2.3f,  -2.0f,  2.4f,  -0.5f,  4.2f,  -0.5f,  1.0f,  0.0f,  -6.0f,
    2.12f,
};

constexpr std::array<float, 12 * 2> house = {
    -1.3f, 0.0f,  -0.93f, 0.0f, -0.93f, 1.15f, 0.93f,  1.15f, 0.93f, 0.0f, 1.3f,  0.0f,
    0.0f,  -1.2f, -1.3f,  0.0f, -0.43f, 0.0f,  -0.43f, .73f,  0.43f, .73f, 0.43f, 0.0f,
};

constexpr std::array<float, 11 * 2> up_arrow_button = {
    9.1f,   -9.1f, 9.1f,   -30.0f, 8.1f,   -30.1f, 7.7f,   -30.1f, -8.6f, -30.0f, -9.0f,
    -29.8f, -9.3f, -29.5f, -9.5f,  -29.1f, -9.1f,  -28.7f, -9.1f,  -9.1f, 0.0f,   0.6f,
};

constexpr std::array<float, 3 * 2> up_arrow_symbol = {
    0.0f, -3.0f, -3.0f, 2.0f, 3.0f, 2.0f,
};

constexpr std::array<float, 13 * 2> up_arrow = {
    9.4f,   -9.8f,  9.4f,   -10.2f, 8.9f,   -29.8f, 8.5f,   -30.0f, 8.1f,
    -30.1f, 7.7f,   -30.1f, -8.6f,  -30.0f, -9.0f,  -29.8f, -9.3f,  -29.5f,
    -9.5f,  -29.1f, -9.5f,  -28.7f, -9.1f,  -9.1f,  -8.8f,  -8.8f,
};

constexpr std::array<float, 64 * 2> trigger_button = {
    5.5f,   -12.6f, 5.8f,   -12.6f, 6.7f,   -12.5f, 8.1f,   -12.3f, 8.6f,   -12.2f, 9.2f,   -12.0f,
    9.5f,   -11.9f, 9.9f,   -11.8f, 10.6f,  -11.5f, 11.0f,  -11.3f, 11.2f,  -11.2f, 11.4f,  -11.1f,
    11.8f,  -10.9f, 12.0f,  -10.8f, 12.2f,  -10.7f, 12.4f,  -10.5f, 12.6f,  -10.4f, 12.8f,  -10.3f,
    13.6f,  -9.7f,  13.8f,  -9.6f,  13.9f,  -9.4f,  14.1f,  -9.3f,  14.8f,  -8.6f,  15.0f,  -8.5f,
    15.1f,  -8.3f,  15.6f,  -7.8f,  15.7f,  -7.6f,  16.1f,  -7.0f,  16.3f,  -6.8f,  16.4f,  -6.6f,
    16.5f,  -6.4f,  16.8f,  -6.0f,  16.9f,  -5.8f,  17.0f,  -5.6f,  17.1f,  -5.4f,  17.2f,  -5.2f,
    17.3f,  -5.0f,  17.4f,  -4.8f,  17.5f,  -4.6f,  17.6f,  -4.4f,  17.7f,  -4.1f,  17.8f,  -3.9f,
    17.9f,  -3.5f,  18.0f,  -3.3f,  18.1f,  -3.0f,  18.2f,  -2.6f,  18.2f,  -2.3f,  18.3f,  -2.1f,
    18.3f,  -1.9f,  18.4f,  -1.4f,  18.5f,  -1.2f,  18.6f,  -0.3f,  18.6f,  0.0f,   18.3f,  13.9f,
    -17.0f, 13.8f,  -17.0f, 13.6f,  -16.4f, -11.4f, -16.3f, -11.6f, -16.1f, -11.8f, -15.7f, -12.0f,
    -15.5f, -12.1f, -15.1f, -12.3f, -14.6f, -12.4f, -13.4f, -12.5f,
};

constexpr std::array<float, 36 * 2> pro_left_trigger = {
    -65.2f,  -132.6f, -68.2f,  -134.1f, -71.3f,  -135.5f, -74.4f,  -136.7f, -77.6f,
    -137.6f, -80.9f,  -138.1f, -84.3f,  -138.3f, -87.6f,  -138.3f, -91.0f,  -138.1f,
    -94.3f,  -137.8f, -97.6f,  -137.3f, -100.9f, -136.7f, -107.5f, -135.3f, -110.7f,
    -134.5f, -120.4f, -131.8f, -123.6f, -130.8f, -126.8f, -129.7f, -129.9f, -128.5f,
    -132.9f, -127.1f, -135.9f, -125.6f, -138.8f, -123.9f, -141.6f, -122.0f, -144.1f,
    -119.8f, -146.3f, -117.3f, -148.4f, -114.7f, -150.4f, -112.0f, -152.3f, -109.2f,
    -155.3f, -104.0f, -152.0f, -104.3f, -148.7f, -104.5f, -145.3f, -104.8f, -35.5f,
    -117.2f, -38.5f,  -118.7f, -41.4f,  -120.3f, -44.4f,  -121.8f, -50.4f,  -124.9f,
};

constexpr std::array<float, 14 * 2> pro_body_top = {
    0.0f,   -115.4f, -4.4f,  -116.1f, -69.7f, -131.3f, -66.4f, -131.9f, -63.1f, -132.3f,
    -56.4f, -133.0f, -53.1f, -133.3f, -49.8f, -133.5f, -43.1f, -133.8f, -39.8f, -134.0f,
    -36.5f, -134.1f, -16.4f, -134.4f, -13.1f, -134.4f, 0.0f,   -134.1f,
};

constexpr std::array<float, 145 * 2> pro_left_handle = {
    -178.7f, -47.5f, -179.0f, -46.1f, -179.3f, -44.6f, -182.0f, -29.8f, -182.3f, -28.4f,
    -182.6f, -26.9f, -182.8f, -25.4f, -183.1f, -23.9f, -183.3f, -22.4f, -183.6f, -21.0f,
    -183.8f, -19.5f, -184.1f, -18.0f, -184.3f, -16.5f, -184.6f, -15.1f, -184.8f, -13.6f,
    -185.1f, -12.1f, -185.3f, -10.6f, -185.6f, -9.1f,  -185.8f, -7.7f,  -186.1f, -6.2f,
    -186.3f, -4.7f,  -186.6f, -3.2f,  -186.8f, -1.7f,  -187.1f, -0.3f,  -187.3f, 1.2f,
    -187.6f, 2.7f,   -187.8f, 4.2f,   -188.3f, 7.1f,   -188.5f, 8.6f,   -188.8f, 10.1f,
    -189.0f, 11.6f,  -189.3f, 13.1f,  -189.5f, 14.5f,  -190.0f, 17.5f,  -190.2f, 19.0f,
    -190.5f, 20.5f,  -190.7f, 21.9f,  -191.2f, 24.9f,  -191.4f, 26.4f,  -191.7f, 27.9f,
    -191.9f, 29.3f,  -192.4f, 32.3f,  -192.6f, 33.8f,  -193.1f, 36.8f,  -193.3f, 38.2f,
    -193.8f, 41.2f,  -194.0f, 42.7f,  -194.7f, 47.1f,  -194.9f, 48.6f,  -199.0f, 82.9f,
    -199.1f, 84.4f,  -199.1f, 85.9f,  -199.2f, 87.4f,  -199.2f, 88.9f,  -199.1f, 94.9f,
    -198.9f, 96.4f,  -198.8f, 97.8f,  -198.5f, 99.3f,  -198.3f, 100.8f, -198.0f, 102.3f,
    -197.7f, 103.7f, -197.4f, 105.2f, -197.0f, 106.7f, -196.6f, 108.1f, -195.7f, 111.0f,
    -195.2f, 112.4f, -194.1f, 115.2f, -193.5f, 116.5f, -192.8f, 117.9f, -192.1f, 119.2f,
    -190.6f, 121.8f, -189.8f, 123.1f, -188.9f, 124.3f, -187.0f, 126.6f, -186.0f, 127.7f,
    -183.9f, 129.8f, -182.7f, 130.8f, -180.3f, 132.6f, -179.1f, 133.4f, -177.8f, 134.1f,
    -176.4f, 134.8f, -175.1f, 135.5f, -173.7f, 136.0f, -169.4f, 137.3f, -167.9f, 137.7f,
    -166.5f, 138.0f, -165.0f, 138.3f, -163.5f, 138.4f, -162.0f, 138.4f, -160.5f, 138.3f,
    -159.0f, 138.0f, -157.6f, 137.7f, -156.1f, 137.3f, -154.7f, 136.9f, -153.2f, 136.5f,
    -151.8f, 136.0f, -150.4f, 135.4f, -149.1f, 134.8f, -147.7f, 134.1f, -146.5f, 133.3f,
    -145.2f, 132.5f, -144.0f, 131.6f, -142.8f, 130.6f, -141.7f, 129.6f, -139.6f, 127.5f,
    -138.6f, 126.4f, -137.7f, 125.2f, -135.1f, 121.5f, -134.3f, 120.3f, -133.5f, 119.0f,
    -131.9f, 116.5f, -131.1f, 115.2f, -128.8f, 111.3f, -128.0f, 110.1f, -127.2f, 108.8f,
    -126.5f, 107.5f, -125.7f, 106.2f, -125.0f, 104.9f, -124.2f, 103.6f, -123.5f, 102.3f,
    -122.0f, 99.6f,  -121.3f, 98.3f,  -115.8f, 87.7f,  -115.1f, 86.4f,  -114.4f, 85.0f,
    -113.7f, 83.7f,  -112.3f, 81.0f,  -111.6f, 79.7f,  -110.1f, 77.1f,  -109.4f, 75.8f,
    -108.0f, 73.1f,  -107.2f, 71.8f,  -106.4f, 70.6f,  -105.7f, 69.3f,  -104.8f, 68.0f,
    -104.0f, 66.8f,  -103.1f, 65.6f,  -101.1f, 63.3f,  -100.0f, 62.3f,  -98.8f,  61.4f,
    -97.6f,  60.6f,  -97.9f,  59.5f,  -98.8f,  58.3f,  -101.5f, 54.6f,  -102.4f, 53.4f,
};

constexpr std::array<float, 245 * 2> pro_body = {
    -0.7f,   -129.1f, -54.3f,  -129.1f, -55.0f,  -129.1f, -57.8f,  -129.0f, -58.5f,  -129.0f,
    -60.7f,  -128.9f, -61.4f,  -128.9f, -62.8f,  -128.8f, -63.5f,  -128.8f, -65.7f,  -128.7f,
    -66.4f,  -128.7f, -67.8f,  -128.6f, -68.5f,  -128.6f, -69.2f,  -128.5f, -70.0f,  -128.5f,
    -70.7f,  -128.4f, -71.4f,  -128.4f, -72.1f,  -128.3f, -72.8f,  -128.3f, -73.5f,  -128.2f,
    -74.2f,  -128.2f, -74.9f,  -128.1f, -75.7f,  -128.1f, -76.4f,  -128.0f, -77.1f,  -128.0f,
    -77.8f,  -127.9f, -78.5f,  -127.9f, -79.2f,  -127.8f, -80.6f,  -127.7f, -81.4f,  -127.6f,
    -82.1f,  -127.5f, -82.8f,  -127.5f, -83.5f,  -127.4f, -84.9f,  -127.3f, -85.6f,  -127.2f,
    -87.0f,  -127.1f, -87.7f,  -127.0f, -88.5f,  -126.9f, -89.2f,  -126.8f, -89.9f,  -126.8f,
    -90.6f,  -126.7f, -94.1f,  -126.3f, -94.8f,  -126.2f, -113.2f, -123.3f, -113.9f, -123.2f,
    -114.6f, -123.0f, -115.3f, -122.9f, -116.7f, -122.6f, -117.4f, -122.5f, -118.1f, -122.3f,
    -118.8f, -122.2f, -119.5f, -122.0f, -120.9f, -121.7f, -121.6f, -121.5f, -122.3f, -121.4f,
    -122.9f, -121.2f, -123.6f, -121.0f, -126.4f, -120.3f, -127.1f, -120.1f, -127.8f, -119.8f,
    -128.4f, -119.6f, -129.1f, -119.4f, -131.2f, -118.7f, -132.5f, -118.3f, -133.2f, -118.0f,
    -133.8f, -117.7f, -134.5f, -117.4f, -135.1f, -117.2f, -135.8f, -116.9f, -136.4f, -116.5f,
    -137.0f, -116.2f, -137.7f, -115.8f, -138.3f, -115.4f, -138.9f, -115.1f, -139.5f, -114.7f,
    -160.0f, -100.5f, -160.5f, -100.0f, -162.5f, -97.9f,  -162.9f, -97.4f,  -163.4f, -96.8f,
    -163.8f, -96.2f,  -165.3f, -93.8f,  -165.7f, -93.2f,  -166.0f, -92.6f,  -166.4f, -91.9f,
    -166.7f, -91.3f,  -167.3f, -90.0f,  -167.6f, -89.4f,  -167.8f, -88.7f,  -168.1f, -88.0f,
    -168.4f, -87.4f,  -168.6f, -86.7f,  -168.9f, -86.0f,  -169.1f, -85.4f,  -169.3f, -84.7f,
    -169.6f, -84.0f,  -169.8f, -83.3f,  -170.2f, -82.0f,  -170.4f, -81.3f,  -172.8f, -72.3f,
    -173.0f, -71.6f,  -173.5f, -69.5f,  -173.7f, -68.8f,  -173.9f, -68.2f,  -174.0f, -67.5f,
    -174.2f, -66.8f,  -174.5f, -65.4f,  -174.7f, -64.7f,  -174.8f, -64.0f,  -175.0f, -63.3f,
    -175.3f, -61.9f,  -175.5f, -61.2f,  -175.8f, -59.8f,  -176.0f, -59.1f,  -176.1f, -58.4f,
    -176.3f, -57.7f,  -176.6f, -56.3f,  -176.8f, -55.6f,  -176.9f, -54.9f,  -177.1f, -54.2f,
    -177.3f, -53.6f,  -177.4f, -52.9f,  -177.6f, -52.2f,  -177.9f, -50.8f,  -178.1f, -50.1f,
    -178.2f, -49.4f,  -178.2f, -48.7f,  -177.8f, -48.1f,  -177.1f, -46.9f,  -176.7f, -46.3f,
    -176.4f, -45.6f,  -176.0f, -45.0f,  -175.3f, -43.8f,  -174.9f, -43.2f,  -174.2f, -42.0f,
    -173.4f, -40.7f,  -173.1f, -40.1f,  -172.7f, -39.5f,  -172.0f, -38.3f,  -171.6f, -37.7f,
    -170.5f, -35.9f,  -170.1f, -35.3f,  -169.7f, -34.6f,  -169.3f, -34.0f,  -168.6f, -32.8f,
    -168.2f, -32.2f,  -166.3f, -29.2f,  -165.9f, -28.6f,  -163.2f, -24.4f,  -162.8f, -23.8f,
    -141.8f, 6.8f,    -141.4f, 7.4f,    -139.4f, 10.3f,   -139.0f, 10.9f,   -138.5f, 11.5f,
    -138.1f, 12.1f,   -137.3f, 13.2f,   -136.9f, 13.8f,   -136.0f, 15.0f,   -135.6f, 15.6f,
    -135.2f, 16.1f,   -134.8f, 16.7f,   -133.9f, 17.9f,   -133.5f, 18.4f,   -133.1f, 19.0f,
    -131.8f, 20.7f,   -131.4f, 21.3f,   -130.1f, 23.0f,   -129.7f, 23.6f,   -128.4f, 25.3f,
    -128.0f, 25.9f,   -126.7f, 27.6f,   -126.3f, 28.2f,   -125.4f, 29.3f,   -125.0f, 29.9f,
    -124.1f, 31.0f,   -123.7f, 31.6f,   -122.8f, 32.7f,   -122.4f, 33.3f,   -121.5f, 34.4f,
    -121.1f, 35.0f,   -120.6f, 35.6f,   -120.2f, 36.1f,   -119.7f, 36.7f,   -119.3f, 37.2f,
    -118.9f, 37.8f,   -118.4f, 38.4f,   -118.0f, 38.9f,   -117.5f, 39.5f,   -117.1f, 40.0f,
    -116.6f, 40.6f,   -116.2f, 41.1f,   -115.7f, 41.7f,   -115.2f, 42.2f,   -114.8f, 42.8f,
    -114.3f, 43.3f,   -113.9f, 43.9f,   -113.4f, 44.4f,   -112.4f, 45.5f,   -112.0f, 46.0f,
    -111.5f, 46.5f,   -110.5f, 47.6f,   -110.0f, 48.1f,   -109.6f, 48.6f,   -109.1f, 49.2f,
    -108.6f, 49.7f,   -107.7f, 50.8f,   -107.2f, 51.3f,   -105.7f, 52.9f,   -105.3f, 53.4f,
    -104.8f, 53.9f,   -104.3f, 54.5f,   -103.8f, 55.0f,   -100.7f, 58.0f,   -100.2f, 58.4f,
    -99.7f,  58.9f,   -99.1f,  59.3f,   -97.2f,  60.3f,   -96.5f,  60.1f,   -95.9f,  59.7f,
    -95.3f,  59.4f,   -94.6f,  59.1f,   -93.9f,  58.9f,   -92.6f,  58.5f,   -91.9f,  58.4f,
    -91.2f,  58.2f,   -90.5f,  58.1f,   -89.7f,  58.0f,   -89.0f,  57.9f,   -86.2f,  57.6f,
    -85.5f,  57.5f,   -84.1f,  57.4f,   -83.4f,  57.3f,   -82.6f,  57.3f,   -81.9f,  57.2f,
    -81.2f,  57.2f,   -80.5f,  57.1f,   -79.8f,  57.1f,   -78.4f,  57.0f,   -77.7f,  57.0f,
    -75.5f,  56.9f,   -74.8f,  56.9f,   -71.9f,  56.8f,   -71.2f,  56.8f,   0.0f,    56.8f,
};

constexpr std::array<float, 84 * 2> left_joycon_body = {
    -145.0f, -78.9f, -145.0f, -77.9f, -145.0f, 85.6f,  -145.0f, 85.6f,  -168.3f, 85.5f,
    -169.3f, 85.4f,  -171.3f, 85.1f,  -172.3f, 84.9f,  -173.4f, 84.7f,  -174.3f, 84.5f,
    -175.3f, 84.2f,  -176.3f, 83.8f,  -177.3f, 83.5f,  -178.2f, 83.1f,  -179.2f, 82.7f,
    -180.1f, 82.2f,  -181.0f, 81.8f,  -181.9f, 81.3f,  -182.8f, 80.7f,  -183.7f, 80.2f,
    -184.5f, 79.6f,  -186.2f, 78.3f,  -186.9f, 77.7f,  -187.7f, 77.0f,  -189.2f, 75.6f,
    -189.9f, 74.8f,  -190.6f, 74.1f,  -191.3f, 73.3f,  -191.9f, 72.5f,  -192.5f, 71.6f,
    -193.1f, 70.8f,  -193.7f, 69.9f,  -194.3f, 69.1f,  -194.8f, 68.2f,  -196.2f, 65.5f,
    -196.6f, 64.5f,  -197.0f, 63.6f,  -197.4f, 62.6f,  -198.1f, 60.7f,  -198.4f, 59.7f,
    -198.6f, 58.7f,  -199.2f, 55.6f,  -199.3f, 54.6f,  -199.5f, 51.5f,  -199.5f, 50.5f,
    -199.5f, -49.4f, -199.4f, -50.5f, -199.3f, -51.5f, -199.1f, -52.5f, -198.2f, -56.5f,
    -197.9f, -57.5f, -197.2f, -59.4f, -196.8f, -60.4f, -196.4f, -61.3f, -195.9f, -62.2f,
    -194.3f, -64.9f, -193.7f, -65.7f, -193.1f, -66.6f, -192.5f, -67.4f, -191.8f, -68.2f,
    -191.2f, -68.9f, -190.4f, -69.7f, -188.2f, -71.8f, -187.4f, -72.5f, -186.6f, -73.1f,
    -185.8f, -73.8f, -185.0f, -74.4f, -184.1f, -74.9f, -183.2f, -75.5f, -182.4f, -76.0f,
    -181.5f, -76.5f, -179.6f, -77.5f, -178.7f, -77.9f, -177.8f, -78.4f, -176.8f, -78.8f,
    -175.9f, -79.1f, -174.9f, -79.5f, -173.9f, -79.8f, -170.9f, -80.6f, -169.9f, -80.8f,
    -167.9f, -81.1f, -166.9f, -81.2f, -165.8f, -81.2f, -145.0f, -80.9f,
};

constexpr std::array<float, 84 * 2> left_joycon_trigger = {
    -166.8f, -83.3f, -167.9f, -83.2f, -168.9f, -83.1f, -170.0f, -83.0f, -171.0f, -82.8f,
    -172.1f, -82.6f, -173.1f, -82.4f, -174.2f, -82.1f, -175.2f, -81.9f, -176.2f, -81.5f,
    -177.2f, -81.2f, -178.2f, -80.8f, -180.1f, -80.0f, -181.1f, -79.5f, -182.0f, -79.0f,
    -183.0f, -78.5f, -183.9f, -78.0f, -184.8f, -77.4f, -185.7f, -76.9f, -186.6f, -76.3f,
    -187.4f, -75.6f, -188.3f, -75.0f, -189.1f, -74.3f, -192.2f, -71.5f, -192.9f, -70.7f,
    -193.7f, -69.9f, -194.3f, -69.1f, -195.0f, -68.3f, -195.6f, -67.4f, -196.8f, -65.7f,
    -197.3f, -64.7f, -197.8f, -63.8f, -198.2f, -62.8f, -198.9f, -60.8f, -198.6f, -59.8f,
    -197.6f, -59.7f, -196.6f, -60.0f, -195.6f, -60.5f, -194.7f, -60.9f, -193.7f, -61.4f,
    -192.8f, -61.9f, -191.8f, -62.4f, -190.9f, -62.8f, -189.9f, -63.3f, -189.0f, -63.8f,
    -187.1f, -64.8f, -186.2f, -65.2f, -185.2f, -65.7f, -184.3f, -66.2f, -183.3f, -66.7f,
    -182.4f, -67.1f, -181.4f, -67.6f, -180.5f, -68.1f, -179.5f, -68.6f, -178.6f, -69.0f,
    -177.6f, -69.5f, -176.7f, -70.0f, -175.7f, -70.5f, -174.8f, -70.9f, -173.8f, -71.4f,
    -172.9f, -71.9f, -171.9f, -72.4f, -171.0f, -72.8f, -170.0f, -73.3f, -169.1f, -73.8f,
    -168.1f, -74.3f, -167.2f, -74.7f, -166.2f, -75.2f, -165.3f, -75.7f, -164.3f, -76.2f,
    -163.4f, -76.6f, -162.4f, -77.1f, -161.5f, -77.6f, -160.5f, -78.1f, -159.6f, -78.5f,
    -158.7f, -79.0f, -157.7f, -79.5f, -156.8f, -80.0f, -155.8f, -80.4f, -154.9f, -80.9f,
    -154.2f, -81.6f, -154.3f, -82.6f, -155.2f, -83.3f, -156.2f, -83.3f,
};

constexpr std::array<float, 70 * 2> handheld_body = {
    -137.3f, -81.9f, -137.6f, -81.8f, -137.8f, -81.6f, -138.0f, -81.3f, -138.1f, -81.1f,
    -138.1f, -80.8f, -138.2f, -78.7f, -138.2f, -78.4f, -138.3f, -78.1f, -138.7f, -77.3f,
    -138.9f, -77.0f, -139.0f, -76.8f, -139.2f, -76.5f, -139.5f, -76.3f, -139.7f, -76.1f,
    -139.9f, -76.0f, -140.2f, -75.8f, -140.5f, -75.7f, -140.7f, -75.6f, -141.0f, -75.5f,
    -141.9f, -75.3f, -142.2f, -75.3f, -142.5f, -75.2f, -143.0f, -74.9f, -143.2f, -74.7f,
    -143.3f, -74.4f, -143.0f, -74.1f, -143.0f, 85.3f,  -143.0f, 85.6f,  -142.7f, 85.8f,
    -142.4f, 85.9f,  -142.2f, 85.9f,  143.0f,  85.6f,  143.1f,  85.4f,  143.3f,  85.1f,
    143.0f,  84.8f,  143.0f,  -74.9f, 142.8f,  -75.1f, 142.5f,  -75.2f, 141.9f,  -75.3f,
    141.6f,  -75.3f, 141.3f,  -75.4f, 141.1f,  -75.4f, 140.8f,  -75.5f, 140.5f,  -75.7f,
    140.2f,  -75.8f, 140.0f,  -76.0f, 139.7f,  -76.1f, 139.5f,  -76.3f, 139.1f,  -76.8f,
    138.9f,  -77.0f, 138.6f,  -77.5f, 138.4f,  -77.8f, 138.3f,  -78.1f, 138.3f,  -78.3f,
    138.2f,  -78.6f, 138.2f,  -78.9f, 138.1f,  -79.2f, 138.1f,  -79.5f, 138.0f,  -81.3f,
    137.8f,  -81.6f, 137.6f,  -81.8f, 137.3f,  -81.9f, 137.1f,  -81.9f, 120.0f,  -70.0f,
    -120.0f, -70.0f, -120.0f, 70.0f,  120.0f,  70.0f,  120.0f,  -70.0f, 137.1f,  -81.9f,
};

constexpr std::array<float, 40 * 2> handheld_bezel = {
    -131.4f, -75.9f, -132.2f, -75.7f, -132.9f, -75.3f, -134.2f, -74.3f, -134.7f, -73.6f,
    -135.1f, -72.8f, -135.4f, -72.0f, -135.5f, -71.2f, -135.5f, -70.4f, -135.2f, 76.7f,
    -134.8f, 77.5f,  -134.3f, 78.1f,  -133.7f, 78.8f,  -133.1f, 79.2f,  -132.3f, 79.6f,
    -131.5f, 79.9f,  -130.7f, 80.0f,  -129.8f, 80.0f,  132.2f,  79.7f,  133.0f,  79.3f,
    133.7f,  78.8f,  134.3f,  78.3f,  134.8f,  77.6f,  135.1f,  76.8f,  135.5f,  75.2f,
    135.5f,  74.3f,  135.2f,  -72.7f, 134.8f,  -73.5f, 134.4f,  -74.2f, 133.8f,  -74.8f,
    133.1f,  -75.3f, 132.3f,  -75.6f, 130.7f,  -76.0f, 129.8f,  -76.0f, -112.9f, -62.2f,
    112.9f,  -62.2f, 112.9f,  62.2f,  -112.9f, 62.2f,  -112.9f, -62.2f, 129.8f,  -76.0f,
};

constexpr std::array<float, 58 * 2> handheld_buttons = {
    -82.48f,  -82.95f, -82.53f,  -82.95f, -106.69f, -82.96f, -106.73f, -82.98f, -106.78f, -83.01f,
    -106.81f, -83.05f, -106.83f, -83.1f,  -106.83f, -83.15f, -106.82f, -83.93f, -106.81f, -83.99f,
    -106.8f,  -84.04f, -106.78f, -84.08f, -106.76f, -84.13f, -106.73f, -84.18f, -106.7f,  -84.22f,
    -106.6f,  -84.34f, -106.56f, -84.37f, -106.51f, -84.4f,  -106.47f, -84.42f, -106.42f, -84.45f,
    -106.37f, -84.47f, -106.32f, -84.48f, -106.17f, -84.5f,  -98.9f,   -84.48f, -98.86f,  -84.45f,
    -98.83f,  -84.41f, -98.81f,  -84.36f, -98.8f,   -84.31f, -98.8f,   -84.26f, -98.79f,  -84.05f,
    -90.26f,  -84.1f,  -90.26f,  -84.15f, -90.25f,  -84.36f, -90.23f,  -84.41f, -90.2f,   -84.45f,
    -90.16f,  -84.48f, -90.11f,  -84.5f,  -82.79f,  -84.49f, -82.74f,  -84.48f, -82.69f,  -84.46f,
    -82.64f,  -84.45f, -82.59f,  -84.42f, -82.55f,  -84.4f,  -82.5f,   -84.37f, -82.46f,  -84.33f,
    -82.42f,  -84.3f,  -82.39f,  -84.26f, -82.3f,   -84.13f, -82.28f,  -84.08f, -82.25f,  -83.98f,
    -82.24f,  -83.93f, -82.23f,  -83.83f, -82.23f,  -83.78f, -82.24f,  -83.1f,  -82.26f,  -83.05f,
    -82.29f,  -83.01f, -82.33f,  -82.97f, -82.38f,  -82.95f,
};

constexpr std::array<float, 47 * 2> left_joycon_slider = {
    -23.7f, -118.2f, -23.7f, -117.3f, -23.7f, 96.6f,   -22.8f, 96.6f,  -21.5f, 97.2f,  -21.5f,
    98.1f,  -21.2f,  106.7f, -20.8f,  107.5f, -20.1f,  108.2f, -19.2f, 108.2f, -16.4f, 108.1f,
    -15.8f, 107.5f,  -15.8f, 106.5f,  -15.8f, 62.8f,   -16.3f, 61.9f,  -15.8f, 61.0f,  -17.3f,
    60.3f,  -19.1f,  58.9f,  -19.1f,  58.1f,  -19.1f,  57.2f,  -19.1f, 34.5f,  -17.9f, 33.9f,
    -17.2f, 33.2f,   -16.6f, 32.4f,   -16.2f, 31.6f,   -15.8f, 30.7f,  -15.8f, 29.7f,  -15.8f,
    28.8f,  -15.8f,  -46.4f, -16.3f,  -47.3f, -15.8f,  -48.1f, -17.4f, -48.8f, -19.1f, -49.4f,
    -19.1f, -50.1f,  -19.1f, -51.0f,  -19.1f, -51.9f,  -19.1f, -73.7f, -19.1f, -74.5f, -17.5f,
    -75.2f, -16.4f,  -76.7f, -16.0f,  -77.6f, -15.8f,  -78.5f, -15.8f, -79.4f, -15.8f, -80.4f,
    -15.8f, -118.2f, -15.8f, -118.2f, -18.3f, -118.2f,
};

constexpr std::array<float, 66 * 2> left_joycon_sideview = {
    -158.8f, -133.5f, -159.8f, -133.5f, -173.5f, -133.3f, -174.5f, -133.0f, -175.4f, -132.6f,
    -176.2f, -132.1f, -177.0f, -131.5f, -177.7f, -130.9f, -178.3f, -130.1f, -179.4f, -128.5f,
    -179.8f, -127.6f, -180.4f, -125.7f, -180.6f, -124.7f, -180.7f, -123.8f, -180.7f, -122.8f,
    -180.0f, 128.8f,  -179.6f, 129.7f,  -179.1f, 130.5f,  -177.9f, 132.1f,  -177.2f, 132.7f,
    -176.4f, 133.3f,  -175.6f, 133.8f,  -174.7f, 134.3f,  -173.8f, 134.6f,  -172.8f, 134.8f,
    -170.9f, 135.0f,  -169.9f, 135.0f,  -156.1f, 134.8f,  -155.2f, 134.6f,  -154.2f, 134.3f,
    -153.3f, 134.0f,  -152.4f, 133.6f,  -151.6f, 133.1f,  -150.7f, 132.6f,  -149.9f, 132.0f,
    -149.2f, 131.4f,  -148.5f, 130.7f,  -147.1f, 129.2f,  -146.5f, 128.5f,  -146.0f, 127.7f,
    -145.5f, 126.8f,  -145.0f, 126.0f,  -144.6f, 125.1f,  -144.2f, 124.1f,  -143.9f, 123.2f,
    -143.7f, 122.2f,  -143.6f, 121.3f,  -143.5f, 120.3f,  -143.5f, 119.3f,  -144.4f, -123.4f,
    -144.8f, -124.3f, -145.3f, -125.1f, -145.8f, -126.0f, -146.3f, -126.8f, -147.0f, -127.5f,
    -147.6f, -128.3f, -148.3f, -129.0f, -149.0f, -129.6f, -149.8f, -130.3f, -150.6f, -130.8f,
    -151.4f, -131.4f, -152.2f, -131.9f, -153.1f, -132.3f, -155.9f, -133.3f, -156.8f, -133.5f,
    -157.8f, -133.5f,
};

constexpr std::array<float, 40 * 2> left_joycon_body_trigger = {
    -146.1f, -124.3f, -146.0f, -122.0f, -145.8f, -119.7f, -145.7f, -117.4f, -145.4f, -112.8f,
    -145.3f, -110.5f, -145.0f, -105.9f, -144.9f, -103.6f, -144.6f, -99.1f,  -144.5f, -96.8f,
    -144.5f, -89.9f,  -144.5f, -87.6f,  -144.5f, -83.0f,  -144.5f, -80.7f,  -144.5f, -80.3f,
    -142.4f, -82.4f,  -141.4f, -84.5f,  -140.2f, -86.4f,  -138.8f, -88.3f,  -137.4f, -90.1f,
    -134.5f, -93.6f,  -133.0f, -95.3f,  -130.0f, -98.8f,  -128.5f, -100.6f, -127.1f, -102.4f,
    -125.8f, -104.3f, -124.7f, -106.3f, -123.9f, -108.4f, -125.1f, -110.2f, -127.4f, -110.3f,
    -129.7f, -110.3f, -134.2f, -110.5f, -136.4f, -111.4f, -138.1f, -112.8f, -139.4f, -114.7f,
    -140.5f, -116.8f, -141.4f, -118.9f, -143.3f, -123.1f, -144.6f, -124.9f, -146.2f, -126.0f,
};

constexpr std::array<float, 49 * 2> left_joycon_topview = {
    -184.8, -20.8,  -185.6, -21.1,  -186.4, -21.5,  -187.1, -22.1,  -187.8, -22.6,  -188.4,
    -23.2,  -189.6, -24.5,  -190.2, -25.2,  -190.7, -25.9,  -191.1, -26.7,  -191.4, -27.5,
    -191.6, -28.4,  -191.7, -29.2,  -191.7, -30.1,  -191.5, -47.7,  -191.2, -48.5,  -191,
    -49.4,  -190.7, -50.2,  -190.3, -51,    -190,   -51.8,  -189.6, -52.6,  -189.1, -53.4,
    -188.6, -54.1,  -187.5, -55.4,  -186.9, -56.1,  -186.2, -56.7,  -185.5, -57.2,  -184,
    -58.1,  -183.3, -58.5,  -182.5, -58.9,  -181.6, -59.2,  -180.8, -59.5,  -179.9, -59.7,
    -179.1, -59.9,  -178.2, -60,    -174.7, -60.1,  -168.5, -60.2,  -162.4, -60.3,  -156.2,
    -60.4,  -149.2, -60.5,  -143,   -60.6,  -136.9, -60.7,  -130.7, -60.8,  -123.7, -60.9,
    -117.5, -61,    -110.5, -61.1,  -94.4,  -60.4,  -94.4,  -59.5,  -94.4,  -20.6,
};

constexpr std::array<float, 41 * 2> left_joycon_slider_topview = {
    -95.1f, -51.5f, -95.0f, -51.5f, -91.2f, -51.6f, -91.2f, -51.7f, -91.1f, -52.4f, -91.1f, -52.6f,
    -91.0f, -54.1f, -86.3f, -54.0f, -86.0f, -53.9f, -85.9f, -53.8f, -85.6f, -53.4f, -85.5f, -53.2f,
    -85.5f, -53.1f, -85.4f, -52.9f, -85.4f, -52.8f, -85.3f, -52.4f, -85.3f, -52.3f, -85.4f, -27.2f,
    -85.4f, -27.1f, -85.5f, -27.0f, -85.5f, -26.9f, -85.6f, -26.7f, -85.6f, -26.6f, -85.7f, -26.5f,
    -85.9f, -26.4f, -86.0f, -26.3f, -86.4f, -26.0f, -86.5f, -25.9f, -86.7f, -25.8f, -87.1f, -25.7f,
    -90.4f, -25.8f, -90.7f, -25.9f, -90.8f, -26.0f, -90.9f, -26.3f, -91.0f, -26.4f, -91.0f, -26.5f,
    -91.1f, -26.7f, -91.1f, -26.9f, -91.2f, -28.9f, -95.2f, -29.1f, -95.2f, -29.2f,
};

constexpr std::array<float, 42 * 2> left_joycon_sideview_zl = {
    -148.9f, -128.2f, -148.7f, -126.6f, -148.4f, -124.9f, -148.2f, -123.3f, -147.9f, -121.7f,
    -147.7f, -120.1f, -147.4f, -118.5f, -147.2f, -116.9f, -146.9f, -115.3f, -146.4f, -112.1f,
    -146.1f, -110.5f, -145.9f, -108.9f, -145.6f, -107.3f, -144.2f, -107.3f, -142.6f, -107.5f,
    -141.0f, -107.8f, -137.8f, -108.3f, -136.2f, -108.6f, -131.4f, -109.4f, -129.8f, -109.7f,
    -125.6f, -111.4f, -124.5f, -112.7f, -123.9f, -114.1f, -123.8f, -115.8f, -123.8f, -117.4f,
    -123.9f, -120.6f, -124.5f, -122.1f, -125.8f, -123.1f, -127.4f, -123.4f, -129.0f, -123.6f,
    -130.6f, -124.0f, -132.1f, -124.4f, -133.7f, -124.8f, -135.3f, -125.3f, -136.8f, -125.9f,
    -138.3f, -126.4f, -139.9f, -126.9f, -141.4f, -127.5f, -142.9f, -128.0f, -144.5f, -128.5f,
    -146.0f, -129.0f, -147.6f, -129.4f,
};

constexpr std::array<float, 72 * 2> left_joystick_sideview = {
    -14.7f, -3.8f,  -15.2f, -5.6f,  -15.2f, -7.6f,  -15.5f, -17.6f, -17.4f, -18.3f, -19.4f, -18.2f,
    -21.3f, -17.6f, -22.8f, -16.4f, -23.4f, -14.5f, -23.4f, -12.5f, -24.1f, -8.6f,  -24.8f, -6.7f,
    -25.3f, -4.8f,  -25.7f, -2.8f,  -25.9f, -0.8f,  -26.0f, 1.2f,   -26.0f, 3.2f,   -25.8f, 5.2f,
    -25.5f, 7.2f,   -25.0f, 9.2f,   -24.4f, 11.1f,  -23.7f, 13.0f,  -23.4f, 14.9f,  -23.4f, 16.9f,
    -23.3f, 18.9f,  -22.0f, 20.5f,  -20.2f, 21.3f,  -18.3f, 21.6f,  -16.3f, 21.4f,  -15.3f, 19.9f,
    -15.3f, 17.8f,  -15.2f, 7.8f,   -13.5f, 6.4f,   -12.4f, 7.2f,   -11.4f, 8.9f,   -10.2f, 10.5f,
    -8.7f,  11.8f,  -7.1f,  13.0f,  -5.3f,  14.0f,  -3.5f,  14.7f,  -1.5f,  15.0f,  0.5f,   15.0f,
    2.5f,   14.7f,  4.4f,   14.2f,  6.3f,   13.4f,  8.0f,   12.4f,  9.6f,   11.1f,  10.9f,  9.6f,
    12.0f,  7.9f,   12.7f,  6.0f,   13.2f,  4.1f,   13.3f,  2.1f,   13.2f,  0.1f,   12.9f,  -1.9f,
    12.2f,  -3.8f,  11.3f,  -5.6f,  10.2f,  -7.2f,  8.8f,   -8.6f,  7.1f,   -9.8f,  5.4f,   -10.8f,
    3.5f,   -11.5f, 1.5f,   -11.9f, -0.5f,  -12.0f, -2.5f,  -11.8f, -4.4f,  -11.3f, -6.2f,  -10.4f,
    -8.0f,  -9.4f,  -9.6f,  -8.2f,  -10.9f, -6.7f,  -11.9f, -4.9f,  -12.8f, -3.2f,  -13.5f, -3.8f,
};

constexpr std::array<float, 63 * 2> left_joystick_L_topview = {
    -186.7f, -43.7f, -186.4f, -43.7f, -110.6f, -43.4f, -110.6f, -43.1f, -110.7f, -34.3f,
    -110.7f, -34.0f, -110.8f, -33.7f, -111.1f, -32.9f, -111.2f, -32.6f, -111.4f, -32.3f,
    -111.5f, -32.1f, -111.7f, -31.8f, -111.8f, -31.5f, -112.0f, -31.3f, -112.2f, -31.0f,
    -112.4f, -30.8f, -112.8f, -30.3f, -113.0f, -30.1f, -114.1f, -29.1f, -114.3f, -28.9f,
    -114.6f, -28.7f, -114.8f, -28.6f, -115.1f, -28.4f, -115.3f, -28.3f, -115.6f, -28.1f,
    -115.9f, -28.0f, -116.4f, -27.8f, -116.7f, -27.7f, -117.3f, -27.6f, -117.6f, -27.5f,
    -182.9f, -27.6f, -183.5f, -27.7f, -183.8f, -27.8f, -184.4f, -27.9f, -184.6f, -28.1f,
    -184.9f, -28.2f, -185.4f, -28.5f, -185.7f, -28.7f, -185.9f, -28.8f, -186.2f, -29.0f,
    -186.4f, -29.2f, -187.0f, -29.9f, -187.2f, -30.1f, -187.6f, -30.6f, -187.8f, -30.8f,
    -187.9f, -31.1f, -188.1f, -31.3f, -188.2f, -31.6f, -188.4f, -31.9f, -188.5f, -32.1f,
    -188.6f, -32.4f, -188.8f, -33.3f, -188.9f, -33.6f, -188.9f, -33.9f, -188.8f, -39.9f,
    -188.8f, -40.2f, -188.7f, -41.1f, -188.7f, -41.4f, -188.6f, -41.7f, -188.0f, -43.1f,
    -187.9f, -43.4f, -187.6f, -43.6f, -187.3f, -43.7f,
};

constexpr std::array<float, 44 * 2> left_joystick_ZL_topview = {
    -179.4f, -53.3f, -177.4f, -53.3f, -111.2f, -53.3f, -111.3f, -53.3f, -111.5f, -58.6f,
    -111.8f, -60.5f, -112.2f, -62.4f, -113.1f, -66.1f, -113.8f, -68.0f, -114.5f, -69.8f,
    -115.3f, -71.5f, -116.3f, -73.2f, -117.3f, -74.8f, -118.5f, -76.4f, -119.8f, -77.8f,
    -121.2f, -79.1f, -122.8f, -80.2f, -124.4f, -81.2f, -126.2f, -82.0f, -128.1f, -82.6f,
    -130.0f, -82.9f, -131.9f, -83.0f, -141.5f, -82.9f, -149.3f, -82.8f, -153.1f, -82.6f,
    -155.0f, -82.1f, -156.8f, -81.6f, -158.7f, -80.9f, -160.4f, -80.2f, -162.2f, -79.3f,
    -163.8f, -78.3f, -165.4f, -77.2f, -166.9f, -76.0f, -168.4f, -74.7f, -169.7f, -73.3f,
    -172.1f, -70.3f, -173.2f, -68.7f, -174.2f, -67.1f, -175.2f, -65.4f, -176.1f, -63.7f,
    -178.7f, -58.5f, -179.6f, -56.8f, -180.4f, -55.1f, -181.3f, -53.3f,
};

void PlayerControlPreview::DrawProBody(QPainter& p, const QPointF center) {
    std::array<QPointF, pro_left_handle.size() / 2> qleft_handle;
    std::array<QPointF, pro_left_handle.size() / 2> qright_handle;
    std::array<QPointF, pro_body.size()> qbody;
    constexpr int radius1 = 32;

    for (std::size_t point = 0; point < pro_left_handle.size() / 2; ++point) {
        qleft_handle[point] =
            center + QPointF(pro_left_handle[point * 2], pro_left_handle[point * 2 + 1]);
        qright_handle[point] =
            center + QPointF(-pro_left_handle[point * 2], pro_left_handle[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < pro_body.size() / 2; ++point) {
        qbody[point] = center + QPointF(pro_body[point * 2], pro_body[point * 2 + 1]);
        qbody[pro_body.size() - 1 - point] =
            center + QPointF(-pro_body[point * 2], pro_body[point * 2 + 1]);
    }

    // Draw left handle body
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_handle);

    // Draw right handle body
    p.setBrush(colors.right);
    DrawPolygon(p, qright_handle);

    // Draw body
    p.setBrush(colors.primary);
    DrawPolygon(p, qbody);

    // Draw joycon circles
    p.setBrush(colors.transparent);
    p.drawEllipse(center + QPoint(-111, -55), radius1, radius1);
    p.drawEllipse(center + QPoint(51, 0), radius1, radius1);
}

void PlayerControlPreview::DrawHandheldBody(QPainter& p, const QPointF center) {
    const std::size_t body_outline_end = handheld_body.size() / 2 - 6;
    const std::size_t bezel_outline_end = handheld_bezel.size() / 2 - 6;
    const std::size_t bezel_inline_size = 4;
    const std::size_t bezel_inline_start = 35;
    std::array<QPointF, left_joycon_body.size() / 2> left_joycon;
    std::array<QPointF, left_joycon_body.size() / 2> right_joycon;
    std::array<QPointF, handheld_body.size() / 2> qhandheld_body;
    std::array<QPointF, body_outline_end> qhandheld_body_outline;
    std::array<QPointF, handheld_bezel.size() / 2> qhandheld_bezel;
    std::array<QPointF, bezel_inline_size> qhandheld_bezel_inline;
    std::array<QPointF, bezel_outline_end> qhandheld_bezel_outline;
    std::array<QPointF, handheld_buttons.size() / 2> qhandheld_buttons;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        left_joycon[point] =
            center + QPointF(left_joycon_body[point * 2], left_joycon_body[point * 2 + 1]);
        right_joycon[point] =
            center + QPointF(-left_joycon_body[point * 2], left_joycon_body[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < body_outline_end; ++point) {
        qhandheld_body_outline[point] =
            center + QPointF(handheld_body[point * 2], handheld_body[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < handheld_body.size() / 2; ++point) {
        qhandheld_body[point] =
            center + QPointF(handheld_body[point * 2], handheld_body[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < handheld_bezel.size() / 2; ++point) {
        qhandheld_bezel[point] =
            center + QPointF(handheld_bezel[point * 2], handheld_bezel[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < bezel_outline_end; ++point) {
        qhandheld_bezel_outline[point] =
            center + QPointF(handheld_bezel[point * 2], handheld_bezel[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < bezel_inline_size; ++point) {
        qhandheld_bezel_inline[point] =
            center + QPointF(handheld_bezel[(point + bezel_inline_start) * 2],
                             handheld_bezel[(point + bezel_inline_start) * 2 + 1]);
    }
    for (std::size_t point = 0; point < handheld_buttons.size() / 2; ++point) {
        qhandheld_buttons[point] =
            center + QPointF(handheld_buttons[point * 2], handheld_buttons[point * 2 + 1]);
    }

    // Draw left joycon
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, left_joycon);

    // Draw right joycon
    p.setPen(colors.outline);
    p.setBrush(colors.right);
    DrawPolygon(p, right_joycon);

    // Draw Handheld buttons
    p.setPen(colors.outline);
    p.setBrush(colors.button);
    DrawPolygon(p, qhandheld_buttons);

    // Draw handheld body
    p.setPen(colors.transparent);
    p.setBrush(colors.primary);
    DrawPolygon(p, qhandheld_body);
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    DrawPolygon(p, qhandheld_body_outline);

    // Draw Handheld bezel
    p.setPen(colors.transparent);
    p.setBrush(colors.button);
    DrawPolygon(p, qhandheld_bezel);
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    DrawPolygon(p, qhandheld_bezel_outline);
    DrawPolygon(p, qhandheld_bezel_inline);
}

void PlayerControlPreview::DrawDualBody(QPainter& p, const QPointF center) {
    std::array<QPointF, left_joycon_body.size() / 2> left_joycon;
    std::array<QPointF, left_joycon_body.size() / 2> right_joycon;
    std::array<QPointF, left_joycon_sideview.size() / 2> qleft_joycon_sideview;
    std::array<QPointF, left_joycon_sideview.size() / 2> qright_joycon_sideview;
    std::array<QPointF, left_joycon_body_trigger.size() / 2> qleft_joycon_trigger;
    std::array<QPointF, left_joycon_body_trigger.size() / 2> qright_joycon_trigger;
    std::array<QPointF, left_joycon_slider.size() / 2> qleft_joycon_slider;
    std::array<QPointF, left_joycon_slider.size() / 2> qright_joycon_slider;
    constexpr float size = 1.61f;
    constexpr float offset = 209.3;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        left_joycon[point] = center + QPointF(left_joycon_body[point * 2] * size + offset,
                                              left_joycon_body[point * 2 + 1] * size - 1);
        right_joycon[point] = center + QPointF(-left_joycon_body[point * 2] * size - offset,
                                               left_joycon_body[point * 2 + 1] * size - 1);
    }
    for (std::size_t point = 0; point < left_joycon_sideview.size() / 2; ++point) {
        qleft_joycon_sideview[point] = center + QPointF(left_joycon_sideview[point * 2],
                                                        left_joycon_sideview[point * 2 + 1] + 2);
        qright_joycon_sideview[point] = center + QPointF(-left_joycon_sideview[point * 2],
                                                         left_joycon_sideview[point * 2 + 1] + 2);
    }
    for (std::size_t point = 0; point < left_joycon_slider.size() / 2; ++point) {
        qleft_joycon_slider[point] =
            center + QPointF(left_joycon_slider[point * 2], left_joycon_slider[point * 2 + 1]);
        qright_joycon_slider[point] =
            center + QPointF(-left_joycon_slider[point * 2], left_joycon_slider[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < left_joycon_body_trigger.size() / 2; ++point) {
        qleft_joycon_trigger[point] = center + QPointF(left_joycon_body_trigger[point * 2],
                                                       left_joycon_body_trigger[point * 2 + 1] + 2);
        qright_joycon_trigger[point] =
            center + QPointF(-left_joycon_body_trigger[point * 2],
                             left_joycon_body_trigger[point * 2 + 1] + 2);
    }

    // right joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.right);
    DrawPolygon(p, right_joycon);
    DrawPolygon(p, qright_joycon_trigger);

    // Left joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, left_joycon);
    DrawPolygon(p, qleft_joycon_trigger);

    // Right Slider release button
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(145, -100), 12, 12, 2);

    // Left Slider release button
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(-145, -100), 12, 12, 2);

    // Right SR and SL sideview buttons
    p.setPen(colors.outline);
    p.setBrush(colors.slider_button);
    DrawRoundRectangle(p, center + QPoint(19, 47), 7, 22, 1);
    DrawRoundRectangle(p, center + QPoint(19, -62), 7, 22, 1);

    // Left SR and SL sideview buttons
    DrawRoundRectangle(p, center + QPoint(-19, 47), 7, 22, 1);
    DrawRoundRectangle(p, center + QPoint(-19, -62), 7, 22, 1);

    // Right Sideview body
    p.setBrush(colors.right);
    DrawPolygon(p, qright_joycon_sideview);
    p.setBrush(colors.slider);
    DrawPolygon(p, qright_joycon_slider);

    // Left Sideview body
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_joycon_sideview);
    p.setBrush(colors.slider);
    DrawPolygon(p, qleft_joycon_slider);

    const QPointF right_sideview_center = QPointF(162.5f, 0) + center;
    const QPointF left_sideview_center = QPointF(-162.5f, 0) + center;

    // right sideview slider body
    p.setBrush(colors.slider);
    DrawRoundRectangle(p, right_sideview_center + QPointF(0, -6), 26, 227, 3);
    p.setBrush(colors.button2);
    DrawRoundRectangle(p, right_sideview_center + QPointF(0, 85), 20.2f, 40.2f, 3);

    // left sideview slider body
    p.setBrush(colors.slider);
    DrawRoundRectangle(p, left_sideview_center + QPointF(0, -6), 26, 227, 3);
    p.setBrush(colors.button2);
    DrawRoundRectangle(p, left_sideview_center + QPointF(0, 85), 20.2f, 40.2f, 3);

    // Right Slider decorations
    p.setPen(colors.outline);
    p.setBrush(colors.slider_arrow);
    DrawArrow(p, right_sideview_center + QPoint(0, 73), Direction::Down, 2.1f);
    DrawArrow(p, right_sideview_center + QPoint(0, 85), Direction::Down, 2.1f);
    DrawArrow(p, right_sideview_center + QPoint(0, 97), Direction::Down, 2.1f);
    DrawCircle(p, right_sideview_center + QPointF(0, 17), 3.8f);

    // Left Slider decorations
    DrawArrow(p, left_sideview_center + QPoint(0, 73), Direction::Down, 2.1f);
    DrawArrow(p, left_sideview_center + QPoint(0, 85), Direction::Down, 2.1f);
    DrawArrow(p, left_sideview_center + QPoint(0, 97), Direction::Down, 2.1f);
    DrawCircle(p, left_sideview_center + QPointF(0, 17), 3.8f);

    // Right SR and SL buttons
    p.setPen(colors.outline);
    p.setBrush(colors.slider_button);
    DrawRoundRectangle(p, right_sideview_center + QPoint(0, 47), 10, 22, 3.6f);
    DrawRoundRectangle(p, right_sideview_center + QPoint(0, -62), 10, 22, 3.6f);

    // Left SR and SL buttons
    DrawRoundRectangle(p, left_sideview_center + QPoint(0, 47), 10, 22, 3.6f);
    DrawRoundRectangle(p, left_sideview_center + QPoint(0, -62), 10, 22, 3.6f);

    // Right SR and SL text
    SetTextFont(p, 5.5f);
    p.setPen(colors.outline);
    p.rotate(-90);
    p.drawText(QPointF(-center.y() - 5, center.x() + 3) + QPointF(-47, 162.5f), tr("SL"));
    p.drawText(QPointF(-center.y() - 5, center.x() + 3) + QPointF(62, 162.5f), tr("SR"));
    p.rotate(90);

    // Left SR and SL text
    p.rotate(90);
    p.drawText(QPointF(center.y() - 5, -center.x() + 3) + QPointF(47, 162.5f), tr("SR"));
    p.drawText(QPointF(center.y() - 5, -center.x() + 3) + QPointF(-62, 162.5f), tr("SL"));
    p.rotate(-90);

    // LED indicators
    const float led_size = 5.0f;
    const QPointF left_led_position = left_sideview_center + QPointF(0, -33);
    const QPointF right_led_position = right_sideview_center + QPointF(0, -33);
    int led_count = 0;
    for (const auto color : led_color) {
        p.setBrush(color);
        DrawRectangle(p, left_led_position + QPointF(0, 11 * led_count), led_size, led_size);
        DrawRectangle(p, right_led_position + QPointF(0, 11 * led_count), led_size, led_size);
        led_count++;
    }
}

void PlayerControlPreview::DrawLeftBody(QPainter& p, const QPointF center) {
    std::array<QPointF, left_joycon_body.size() / 2> left_joycon;
    std::array<QPointF, left_joycon_sideview.size() / 2> qleft_joycon_sideview;
    std::array<QPointF, left_joycon_body_trigger.size() / 2> qleft_joycon_trigger;
    std::array<QPointF, left_joycon_slider.size() / 2> qleft_joycon_slider;
    std::array<QPointF, left_joycon_slider_topview.size() / 2> qleft_joycon_slider_topview;
    std::array<QPointF, left_joycon_topview.size() / 2> qleft_joycon_topview;
    constexpr float size = 1.78f;
    constexpr float size2 = 1.1115f;
    constexpr float offset = 312.39f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        left_joycon[point] = center + QPointF(left_joycon_body[point * 2] * size + offset,
                                              left_joycon_body[point * 2 + 1] * size - 1);
    }

    for (std::size_t point = 0; point < left_joycon_sideview.size() / 2; ++point) {
        qleft_joycon_sideview[point] =
            center + QPointF(left_joycon_sideview[point * 2] * size2 + offset2,
                             left_joycon_sideview[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_slider.size() / 2; ++point) {
        qleft_joycon_slider[point] = center + QPointF(left_joycon_slider[point * 2] * size2 + 81,
                                                      left_joycon_slider[point * 2 + 1] * size2);
    }
    for (std::size_t point = 0; point < left_joycon_body_trigger.size() / 2; ++point) {
        qleft_joycon_trigger[point] =
            center + QPointF(left_joycon_body_trigger[point * 2] * size2 + offset2,
                             left_joycon_body_trigger[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_topview.size() / 2; ++point) {
        qleft_joycon_topview[point] =
            center + QPointF(left_joycon_topview[point * 2], left_joycon_topview[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < left_joycon_slider_topview.size() / 2; ++point) {
        qleft_joycon_slider_topview[point] =
            center + QPointF(left_joycon_slider_topview[point * 2],
                             left_joycon_slider_topview[point * 2 + 1]);
    }

    // Joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, left_joycon);
    DrawPolygon(p, qleft_joycon_trigger);

    // Slider release button top view
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(-107, -62), 14, 12, 2);

    // Joycon slider top view
    p.setBrush(colors.slider);
    DrawPolygon(p, qleft_joycon_slider_topview);
    p.drawLine(center + QPointF(-91.1f, -51.7f), center + QPointF(-91.1f, -26.5f));

    // Joycon body top view
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_joycon_topview);

    // Slider release button
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(175, -110), 12, 14, 2);

    // Sideview body
    p.setBrush(colors.left);
    DrawPolygon(p, qleft_joycon_sideview);
    p.setBrush(colors.slider);
    DrawPolygon(p, qleft_joycon_slider);

    const QPointF sideview_center = QPointF(155, 0) + center;

    // Sideview slider body
    p.setBrush(colors.slider);
    DrawRoundRectangle(p, sideview_center + QPointF(0, -5), 28, 253, 3);
    p.setBrush(colors.button2);
    DrawRoundRectangle(p, sideview_center + QPointF(0, 97), 22.44f, 44.66f, 3);

    // Slider decorations
    p.setPen(colors.outline);
    p.setBrush(colors.slider_arrow);
    DrawArrow(p, sideview_center + QPoint(0, 83), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 96), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 109), Direction::Down, 2.2f);
    DrawCircle(p, sideview_center + QPointF(0, 19), 4.44f);

    // LED indicators
    const float led_size = 5.0f;
    const QPointF led_position = sideview_center + QPointF(0, -36);
    int led_count = 0;
    for (const auto color : led_color) {
        p.setBrush(color);
        DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    }
}

void PlayerControlPreview::DrawRightBody(QPainter& p, const QPointF center) {
    std::array<QPointF, left_joycon_body.size() / 2> right_joycon;
    std::array<QPointF, left_joycon_sideview.size() / 2> qright_joycon_sideview;
    std::array<QPointF, left_joycon_body_trigger.size() / 2> qright_joycon_trigger;
    std::array<QPointF, left_joycon_slider.size() / 2> qright_joycon_slider;
    std::array<QPointF, left_joycon_slider_topview.size() / 2> qright_joycon_slider_topview;
    std::array<QPointF, left_joycon_topview.size() / 2> qright_joycon_topview;
    constexpr float size = 1.78f;
    constexpr float size2 = 1.1115f;
    constexpr float offset = 312.39f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_body.size() / 2; ++point) {
        right_joycon[point] = center + QPointF(-left_joycon_body[point * 2] * size - offset,
                                               left_joycon_body[point * 2 + 1] * size - 1);
    }

    for (std::size_t point = 0; point < left_joycon_sideview.size() / 2; ++point) {
        qright_joycon_sideview[point] =
            center + QPointF(-left_joycon_sideview[point * 2] * size2 - offset2,
                             left_joycon_sideview[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_body_trigger.size() / 2; ++point) {
        qright_joycon_trigger[point] =
            center + QPointF(-left_joycon_body_trigger[point * 2] * size2 - offset2,
                             left_joycon_body_trigger[point * 2 + 1] * size2 + 2);
    }
    for (std::size_t point = 0; point < left_joycon_slider.size() / 2; ++point) {
        qright_joycon_slider[point] = center + QPointF(-left_joycon_slider[point * 2] * size2 - 81,
                                                       left_joycon_slider[point * 2 + 1] * size2);
    }
    for (std::size_t point = 0; point < left_joycon_topview.size() / 2; ++point) {
        qright_joycon_topview[point] =
            center + QPointF(-left_joycon_topview[point * 2], left_joycon_topview[point * 2 + 1]);
    }
    for (std::size_t point = 0; point < left_joycon_slider_topview.size() / 2; ++point) {
        qright_joycon_slider_topview[point] =
            center + QPointF(-left_joycon_slider_topview[point * 2],
                             left_joycon_slider_topview[point * 2 + 1]);
    }

    // Joycon body
    p.setPen(colors.outline);
    p.setBrush(colors.left);
    DrawPolygon(p, right_joycon);
    DrawPolygon(p, qright_joycon_trigger);

    // Slider release button top view
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(107, -62), 14, 12, 2);

    // Joycon slider top view
    p.setBrush(colors.slider);
    DrawPolygon(p, qright_joycon_slider_topview);
    p.drawLine(center + QPointF(91.1f, -51.7f), center + QPointF(91.1f, -26.5f));

    // Joycon body top view
    p.setBrush(colors.left);
    DrawPolygon(p, qright_joycon_topview);

    // Slider release button
    p.setBrush(colors.button);
    DrawRoundRectangle(p, center + QPoint(-175, -110), 12, 14, 2);

    // Sideview body
    p.setBrush(colors.left);
    DrawPolygon(p, qright_joycon_sideview);
    p.setBrush(colors.slider);
    DrawPolygon(p, qright_joycon_slider);

    const QPointF sideview_center = QPointF(-155, 0) + center;

    // Sideview slider body
    p.setBrush(colors.slider);
    DrawRoundRectangle(p, sideview_center + QPointF(0, -5), 28, 253, 3);
    p.setBrush(colors.button2);
    DrawRoundRectangle(p, sideview_center + QPointF(0, 97), 22.44f, 44.66f, 3);

    // Slider decorations
    p.setPen(colors.outline);
    p.setBrush(colors.slider_arrow);
    DrawArrow(p, sideview_center + QPoint(0, 83), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 96), Direction::Down, 2.2f);
    DrawArrow(p, sideview_center + QPoint(0, 109), Direction::Down, 2.2f);
    DrawCircle(p, sideview_center + QPointF(0, 19), 4.44f);

    // LED indicators
    const float led_size = 5.0f;
    const QPointF led_position = sideview_center + QPointF(0, -36);
    int led_count = 0;
    for (const auto color : led_color) {
        p.setBrush(color);
        DrawRectangle(p, led_position + QPointF(0, 12 * led_count++), led_size, led_size);
    }
}

void PlayerControlPreview::DrawProTriggers(QPainter& p, const QPointF center, bool left_pressed,
                                           bool right_pressed) {
    std::array<QPointF, pro_left_trigger.size() / 2> qleft_trigger;
    std::array<QPointF, pro_left_trigger.size() / 2> qright_trigger;
    std::array<QPointF, pro_body_top.size()> qbody_top;

    for (std::size_t point = 0; point < pro_left_trigger.size() / 2; ++point) {
        qleft_trigger[point] =
            center + QPointF(pro_left_trigger[point * 2],
                             pro_left_trigger[point * 2 + 1] + (left_pressed ? 2 : 0));
        qright_trigger[point] =
            center + QPointF(-pro_left_trigger[point * 2],
                             pro_left_trigger[point * 2 + 1] + (right_pressed ? 2 : 0));
    }

    for (std::size_t point = 0; point < pro_body_top.size() / 2; ++point) {
        qbody_top[pro_body_top.size() - 1 - point] =
            center + QPointF(-pro_body_top[point * 2], pro_body_top[point * 2 + 1]);
        qbody_top[point] = center + QPointF(pro_body_top[point * 2], pro_body_top[point * 2 + 1]);
    }

    // Pro body detail
    p.setPen(colors.outline);
    p.setBrush(colors.primary);
    DrawPolygon(p, qbody_top);

    // Left trigger
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Right trigger
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawHandheldTriggers(QPainter& p, const QPointF center,
                                                bool left_pressed, bool right_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qleft_trigger;
    std::array<QPointF, left_joycon_trigger.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        qleft_trigger[point] =
            center + QPointF(left_joycon_trigger[point * 2],
                             left_joycon_trigger[point * 2 + 1] + (left_pressed ? 0.5f : 0));
        qright_trigger[point] =
            center + QPointF(-left_joycon_trigger[point * 2],
                             left_joycon_trigger[point * 2 + 1] + (right_pressed ? 0.5f : 0));
    }

    // Left trigger
    p.setPen(colors.outline);
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Right trigger
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawDualTriggers(QPainter& p, const QPointF center, bool left_pressed,
                                            bool right_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qleft_trigger;
    std::array<QPointF, left_joycon_trigger.size() / 2> qright_trigger;
    constexpr float size = 1.62f;
    constexpr float offset = 210.6f;
    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        qleft_trigger[point] =
            center + QPointF(left_joycon_trigger[point * 2] * size + offset,
                             left_joycon_trigger[point * 2 + 1] * size + (left_pressed ? 0.5f : 0));
        qright_trigger[point] = center + QPointF(-left_joycon_trigger[point * 2] * size - offset,
                                                 left_joycon_trigger[point * 2 + 1] * size +
                                                     (right_pressed ? 0.5f : 0));
    }

    // Left trigger
    p.setPen(colors.outline);
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Right trigger
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawDualZTriggers(QPainter& p, const QPointF center, bool left_pressed,
                                             bool right_pressed) {
    std::array<QPointF, left_joycon_sideview_zl.size() / 2> qleft_trigger;
    std::array<QPointF, left_joycon_sideview_zl.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_joycon_sideview_zl.size() / 2; ++point) {
        qleft_trigger[point] =
            center + QPointF(left_joycon_sideview_zl[point * 2],
                             left_joycon_sideview_zl[point * 2 + 1] + (left_pressed ? 2.5f : 2.0f));
        qright_trigger[point] = center + QPointF(-left_joycon_sideview_zl[point * 2],
                                                 left_joycon_sideview_zl[point * 2 + 1] +
                                                     (right_pressed ? 2.5f : 2.0f));
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
    p.drawArc(center.x() - 159, center.y() - 183 + (left_pressed ? 0.5f : 0), 70, 70, 225 * 16,
              44 * 16);
    p.drawArc(center.x() + 90, center.y() - 183 + (right_pressed ? 0.5f : 0), 70, 70, 271 * 16,
              44 * 16);
}

void PlayerControlPreview::DrawLeftTriggers(QPainter& p, const QPointF center, bool left_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qleft_trigger;
    constexpr float size = 1.78f;
    constexpr float offset = 311.5f;

    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joycon_trigger[point * 2] * size + offset,
                                                left_joycon_trigger[point * 2 + 1] * size -
                                                    (left_pressed ? 0.5f : 1.0f));
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);
}

void PlayerControlPreview::DrawLeftZTriggers(QPainter& p, const QPointF center, bool left_pressed) {
    std::array<QPointF, left_joycon_sideview_zl.size() / 2> qleft_trigger;
    constexpr float size = 1.1115f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_sideview_zl.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joycon_sideview_zl[point * 2] * size + offset2,
                                                left_joycon_sideview_zl[point * 2 + 1] * size +
                                                    (left_pressed ? 1.5f : 1.0f));
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);
    p.drawArc(center.x() + 158, center.y() + (left_pressed ? -203.5f : -204.0f), 77, 77, 225 * 16,
              44 * 16);
}

void PlayerControlPreview::DrawLeftTriggersTopView(QPainter& p, const QPointF center,
                                                   bool left_pressed) {
    std::array<QPointF, left_joystick_L_topview.size() / 2> qleft_trigger;

    for (std::size_t point = 0; point < left_joystick_L_topview.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joystick_L_topview[point * 2],
                                                left_joystick_L_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Draw ZL text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(-143, -36), Symbol::ZL, 1.0f);

    // Delete Z character
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawRectangle(p, center + QPointF(-146, -36), 6, 10);
}

void PlayerControlPreview::DrawLeftZTriggersTopView(QPainter& p, const QPointF center,
                                                    bool left_pressed) {
    std::array<QPointF, left_joystick_ZL_topview.size() / 2> qleft_trigger;

    for (std::size_t point = 0; point < left_joystick_ZL_topview.size() / 2; ++point) {
        qleft_trigger[point] = center + QPointF(left_joystick_ZL_topview[point * 2],
                                                left_joystick_ZL_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(left_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qleft_trigger);

    // Draw ZL text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(-140, -68), Symbol::ZL, 1.0f);
}

void PlayerControlPreview::DrawRightTriggers(QPainter& p, const QPointF center,
                                             bool right_pressed) {
    std::array<QPointF, left_joycon_trigger.size() / 2> qright_trigger;
    constexpr float size = 1.78f;
    constexpr float offset = 311.5f;

    for (std::size_t point = 0; point < left_joycon_trigger.size() / 2; ++point) {
        qright_trigger[point] = center + QPointF(-left_joycon_trigger[point * 2] * size - offset,
                                                 left_joycon_trigger[point * 2 + 1] * size -
                                                     (right_pressed ? 0.5f : 1.0f));
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
}

void PlayerControlPreview::DrawRightZTriggers(QPainter& p, const QPointF center,
                                              bool right_pressed) {
    std::array<QPointF, left_joycon_sideview_zl.size() / 2> qright_trigger;
    constexpr float size = 1.1115f;
    constexpr float offset2 = 335;

    for (std::size_t point = 0; point < left_joycon_sideview_zl.size() / 2; ++point) {
        qright_trigger[point] =
            center +
            QPointF(-left_joycon_sideview_zl[point * 2] * size - offset2,
                    left_joycon_sideview_zl[point * 2 + 1] * size + (right_pressed ? 0.5f : 0) + 1);
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);
    p.drawArc(center.x() - 236, center.y() + (right_pressed ? -203.5f : -204.0f), 77, 77, 271 * 16,
              44 * 16);
}

void PlayerControlPreview::DrawRightTriggersTopView(QPainter& p, const QPointF center,
                                                    bool right_pressed) {
    std::array<QPointF, left_joystick_L_topview.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_joystick_L_topview.size() / 2; ++point) {
        qright_trigger[point] = center + QPointF(-left_joystick_L_topview[point * 2],
                                                 left_joystick_L_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);

    // Draw ZR text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(137, -36), Symbol::ZR, 1.0f);

    // Delete Z character
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawRectangle(p, center + QPointF(134, -36), 6, 10);
}

void PlayerControlPreview::DrawRightZTriggersTopView(QPainter& p, const QPointF center,
                                                     bool right_pressed) {
    std::array<QPointF, left_joystick_ZL_topview.size() / 2> qright_trigger;

    for (std::size_t point = 0; point < left_joystick_ZL_topview.size() / 2; ++point) {
        qright_trigger[point] = center + QPointF(-left_joystick_ZL_topview[point * 2],
                                                 left_joystick_ZL_topview[point * 2 + 1]);
    }

    p.setPen(colors.outline);
    p.setBrush(right_pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qright_trigger);

    // Draw ZR text
    p.setPen(colors.transparent);
    p.setBrush(colors.font2);
    DrawSymbol(p, center + QPointF(140, -68), Symbol::ZR, 1.0f);
}

void PlayerControlPreview::DrawJoystick(QPainter& p, const QPointF center, float size,
                                        bool pressed) {
    const float radius1 = 13.0f * size;
    const float radius2 = 9.0f * size;

    // Outer circle
    p.setPen(colors.outline);
    p.setBrush(pressed ? colors.highlight : colors.button);
    DrawCircle(p, center, radius1);

    // Cross
    p.drawLine(center - QPoint(radius1, 0), center + QPoint(radius1, 0));
    p.drawLine(center - QPoint(0, radius1), center + QPoint(0, radius1));

    // Inner circle
    p.setBrush(pressed ? colors.highlight2 : colors.button2);
    DrawCircle(p, center, radius2);
}

void PlayerControlPreview::DrawJoystickSideview(QPainter& p, const QPointF center, float angle,
                                                float size, bool pressed) {
    QVector<QPointF> joystick;
    joystick.reserve(left_joystick_sideview.size() / 2);

    for (std::size_t point = 0; point < left_joystick_sideview.size() / 2; ++point) {
        joystick.append(QPointF(left_joystick_sideview[point * 2] * size + (pressed ? 1 : 0),
                                left_joystick_sideview[point * 2 + 1] * size - 1));
    }

    // Rotate joystick
    QTransform t;
    t.translate(center.x(), center.y());
    t.rotate(18 * angle);
    QPolygonF p2 = t.map(QPolygonF(joystick));

    // Draw joystick
    p.setPen(colors.outline);
    p.setBrush(pressed ? colors.highlight : colors.button);
    p.drawPolygon(p2);
    p.drawLine(p2.at(1), p2.at(30));
    p.drawLine(p2.at(32), p2.at(71));
}

void PlayerControlPreview::DrawProJoystick(QPainter& p, const QPointF center, bool pressed) {
    // Outer circle
    p.setPen(colors.outline);
    p.setBrush(pressed ? colors.highlight : colors.button);
    DrawCircle(p, center, 24.0f);

    // Inner circle
    p.setBrush(pressed ? colors.highlight2 : colors.button2);
    DrawCircle(p, center, 17.0f);
}

void PlayerControlPreview::DrawRawJoystick(QPainter& p, const QPointF center, const QPointF value,
                                           const Input::AnalogProperties properties) {
    constexpr float size = 45.0f;
    const float range = size * properties.range;
    const float deadzone = size * properties.deadzone;

    // Max range zone circle
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    QPen pen = p.pen();
    pen.setStyle(Qt::DotLine);
    p.setPen(pen);
    DrawCircle(p, center, range);

    // Deadzone circle
    pen.setColor(colors.deadzone);
    p.setPen(pen);
    DrawCircle(p, center, deadzone);

    // Dot pointer
    p.setPen(colors.indicator);
    p.setBrush(colors.indicator);
    DrawCircle(p, center + (value * range), 2);
}

void PlayerControlPreview::DrawRoundButton(QPainter& p, QPointF center, bool pressed, float width,
                                           float height, Direction direction, float radius) {
    p.setBrush(button_color);
    if (pressed) {
        switch (direction) {
        case Direction::Left:
            center.setX(center.x() - 1);
            break;
        case Direction::Right:
            center.setX(center.x() + 1);
            break;
        case Direction::Down:
            center.setY(center.y() + 1);
            break;
        case Direction::Up:
            center.setY(center.y() - 1);
            break;
        case Direction::None:
            break;
        }
        p.setBrush(colors.highlight);
    }
    QRectF rect = {center.x() - width, center.y() - height, width * 2.0f, height * 2.0f};
    p.drawRoundedRect(rect, radius, radius);
}
void PlayerControlPreview::DrawMinusButton(QPainter& p, const QPointF center, bool pressed,
                                           int button_size) {
    p.setPen(colors.outline);
    p.setBrush(pressed ? colors.highlight : colors.button);
    DrawRectangle(p, center, button_size, button_size / 3.0f);
}
void PlayerControlPreview::DrawPlusButton(QPainter& p, const QPointF center, bool pressed,
                                          int button_size) {
    // Draw outer line
    p.setPen(colors.outline);
    p.setBrush(pressed ? colors.highlight : colors.button);
    DrawRectangle(p, center, button_size, button_size / 3.0f);
    DrawRectangle(p, center, button_size / 3.0f, button_size);

    // Scale down size
    button_size *= 0.88f;

    // Draw inner color
    p.setPen(colors.transparent);
    DrawRectangle(p, center, button_size, button_size / 3.0f);
    DrawRectangle(p, center, button_size / 3.0f, button_size);
}

void PlayerControlPreview::DrawCircleButton(QPainter& p, const QPointF center, bool pressed,
                                            float button_size) {
    p.setBrush(button_color);
    if (pressed) {
        p.setBrush(colors.highlight);
    }
    p.drawEllipse(center, button_size, button_size);
}

void PlayerControlPreview::DrawArrowButtonOutline(QPainter& p, const QPointF center) {
    const std::size_t arrow_points = up_arrow_button.size() / 2;
    std::array<QPointF, (arrow_points - 1) * 4> arrow_button_outline;

    for (std::size_t point = 0; point < arrow_points - 1; ++point) {
        arrow_button_outline[point] =
            center + QPointF(up_arrow_button[point * 2], up_arrow_button[point * 2 + 1]);
        arrow_button_outline[(arrow_points - 1) * 2 - point - 1] =
            center + QPointF(up_arrow_button[point * 2 + 1], up_arrow_button[point * 2]);
        arrow_button_outline[(arrow_points - 1) * 2 + point] =
            center + QPointF(-up_arrow_button[point * 2], -up_arrow_button[point * 2 + 1]);
        arrow_button_outline[(arrow_points - 1) * 4 - point - 1] =
            center + QPointF(-up_arrow_button[point * 2 + 1], -up_arrow_button[point * 2]);
    }
    // Draw arrow button outline
    p.setPen(colors.outline);
    p.setBrush(colors.transparent);
    DrawPolygon(p, arrow_button_outline);
}

void PlayerControlPreview::DrawArrowButton(QPainter& p, const QPointF center,
                                           const Direction direction, bool pressed) {
    std::array<QPointF, up_arrow_button.size() / 2> arrow_button;
    QPoint offset;

    for (std::size_t point = 0; point < up_arrow_button.size() / 2; ++point) {
        switch (direction) {
        case Direction::Up:
            arrow_button[point] =
                center + QPointF(up_arrow_button[point * 2], up_arrow_button[point * 2 + 1]);
            break;
        case Direction::Left:
            arrow_button[point] =
                center + QPointF(up_arrow_button[point * 2 + 1], up_arrow_button[point * 2]);
            break;
        case Direction::Right:
            arrow_button[point] =
                center + QPointF(-up_arrow_button[point * 2 + 1], up_arrow_button[point * 2]);
            break;
        case Direction::Down:
            arrow_button[point] =
                center + QPointF(up_arrow_button[point * 2], -up_arrow_button[point * 2 + 1]);
            break;
        case Direction::None:
            break;
        }
    }

    // Draw arrow button
    p.setPen(pressed ? colors.highlight : colors.button);
    p.setBrush(pressed ? colors.highlight : colors.button);
    DrawPolygon(p, arrow_button);

    switch (direction) {
    case Direction::Up:
        offset = QPoint(0, -20);
        break;
    case Direction::Left:
        offset = QPoint(-20, 0);
        break;
    case Direction::Right:
        offset = QPoint(20, 0);
        break;
    case Direction::Down:
        offset = QPoint(0, 20);
        break;
    case Direction::None:
        offset = QPoint(0, 0);
        break;
    }

    // Draw arrow icon
    p.setPen(colors.font2);
    p.setBrush(colors.font2);
    DrawArrow(p, center + offset, direction, 1.0f);
}

void PlayerControlPreview::DrawTriggerButton(QPainter& p, const QPointF center,
                                             const Direction direction, bool pressed) {
    std::array<QPointF, trigger_button.size() / 2> qtrigger_button;
    QPoint offset;

    for (std::size_t point = 0; point < trigger_button.size() / 2; ++point) {
        switch (direction) {
        case Direction::Left:
            qtrigger_button[point] =
                center + QPointF(-trigger_button[point * 2], trigger_button[point * 2 + 1]);
            break;
        case Direction::Right:
            qtrigger_button[point] =
                center + QPointF(trigger_button[point * 2], trigger_button[point * 2 + 1]);
            break;
        case Direction::Up:
        case Direction::Down:
        case Direction::None:
            break;
        }
    }

    // Draw arrow button
    p.setPen(colors.outline);
    p.setBrush(pressed ? colors.highlight : colors.button);
    DrawPolygon(p, qtrigger_button);
}

void PlayerControlPreview::DrawSymbol(QPainter& p, const QPointF center, Symbol symbol,
                                      float icon_size) {
    std::array<QPointF, house.size() / 2> house_icon;
    std::array<QPointF, symbol_a.size() / 2> a_icon;
    std::array<QPointF, symbol_b.size() / 2> b_icon;
    std::array<QPointF, symbol_x.size() / 2> x_icon;
    std::array<QPointF, symbol_y.size() / 2> y_icon;
    std::array<QPointF, symbol_zl.size() / 2> zl_icon;
    std::array<QPointF, symbol_zr.size() / 2> zr_icon;
    switch (symbol) {
    case Symbol::House:
        for (std::size_t point = 0; point < house.size() / 2; ++point) {
            house_icon[point] = center + QPointF(house[point * 2] * icon_size,
                                                 (house[point * 2 + 1] - 0.025f) * icon_size);
        }
        p.drawPolygon(house_icon.data(), static_cast<int>(house_icon.size()));
        break;
    case Symbol::A:
        for (std::size_t point = 0; point < symbol_a.size() / 2; ++point) {
            a_icon[point] = center + QPointF(symbol_a[point * 2] * icon_size,
                                             symbol_a[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(a_icon.data(), static_cast<int>(a_icon.size()));
        break;
    case Symbol::B:
        for (std::size_t point = 0; point < symbol_b.size() / 2; ++point) {
            b_icon[point] = center + QPointF(symbol_b[point * 2] * icon_size,
                                             symbol_b[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(b_icon.data(), static_cast<int>(b_icon.size()));
        break;
    case Symbol::X:
        for (std::size_t point = 0; point < symbol_x.size() / 2; ++point) {
            x_icon[point] = center + QPointF(symbol_x[point * 2] * icon_size,
                                             symbol_x[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(x_icon.data(), static_cast<int>(x_icon.size()));
        break;
    case Symbol::Y:
        for (std::size_t point = 0; point < symbol_y.size() / 2; ++point) {
            y_icon[point] = center + QPointF(symbol_y[point * 2] * icon_size,
                                             (symbol_y[point * 2 + 1] - 1.0f) * icon_size);
        }
        p.drawPolygon(y_icon.data(), static_cast<int>(y_icon.size()));
        break;
    case Symbol::ZL:
        for (std::size_t point = 0; point < symbol_zl.size() / 2; ++point) {
            zl_icon[point] = center + QPointF(symbol_zl[point * 2] * icon_size,
                                              symbol_zl[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(zl_icon.data(), static_cast<int>(zl_icon.size()));
        break;
    case Symbol::ZR:
        for (std::size_t point = 0; point < symbol_zr.size() / 2; ++point) {
            zr_icon[point] = center + QPointF(symbol_zr[point * 2] * icon_size,
                                              symbol_zr[point * 2 + 1] * icon_size);
        }
        p.drawPolygon(zr_icon.data(), static_cast<int>(zr_icon.size()));
        break;
    }
}

void PlayerControlPreview::DrawArrow(QPainter& p, const QPointF center, const Direction direction,
                                     float size) {

    std::array<QPointF, up_arrow_symbol.size() / 2> arrow_symbol;

    for (std::size_t point = 0; point < up_arrow_symbol.size() / 2; ++point) {
        switch (direction) {
        case Direction::Up:
            arrow_symbol[point] = center + QPointF(up_arrow_symbol[point * 2] * size,
                                                   up_arrow_symbol[point * 2 + 1] * size);
            break;
        case Direction::Left:
            arrow_symbol[point] = center + QPointF(up_arrow_symbol[point * 2 + 1] * size,
                                                   up_arrow_symbol[point * 2] * size);
            break;
        case Direction::Right:
            arrow_symbol[point] = center + QPointF(-up_arrow_symbol[point * 2 + 1] * size,
                                                   up_arrow_symbol[point * 2] * size);
            break;
        case Direction::Down:
            arrow_symbol[point] = center + QPointF(up_arrow_symbol[point * 2] * size,
                                                   -up_arrow_symbol[point * 2 + 1] * size);
            break;
        case Direction::None:
            break;
        }
    }

    DrawPolygon(p, arrow_symbol);
}

template <size_t N>
void PlayerControlPreview::DrawPolygon(QPainter& p, const std::array<QPointF, N>& polygon) {
    p.drawPolygon(polygon.data(), static_cast<int>(polygon.size()));
}

void PlayerControlPreview::DrawCircle(QPainter& p, const QPointF center, float size) {
    p.drawEllipse(center, size, size);
}

void PlayerControlPreview::DrawRectangle(QPainter& p, const QPointF center, float width,
                                         float height) {
    const QRectF rect = QRectF(center.x() - (width / 2), center.y() - (height / 2), width, height);
    p.drawRect(rect);
}
void PlayerControlPreview::DrawRoundRectangle(QPainter& p, const QPointF center, float width,
                                              float height, float round) {
    const QRectF rect = QRectF(center.x() - (width / 2), center.y() - (height / 2), width, height);
    p.drawRoundedRect(rect, round, round);
}

void PlayerControlPreview::DrawText(QPainter& p, const QPointF center, float text_size,
                                    const QString& text) {
    SetTextFont(p, text_size);
    const QFontMetrics fm(p.font());
    const QPointF offset = {fm.width(text) / 2.0f, -text_size / 2.0f};
    p.drawText(center - offset, text);
}

void PlayerControlPreview::SetTextFont(QPainter& p, float text_size, const QString& font_family) {
    QFont font = p.font();
    font.setPointSizeF(text_size);
    font.setFamily(font_family);
    p.setFont(font);
}
