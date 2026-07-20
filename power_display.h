// power_display.h
// Round 800x800 power-flow dashboard for the Waveshare ESP32-P4 3.4" display.
// One glanceable page, nothing to tap through:
//   - outer ring: live solar/grid/battery/home power flow (gauge arcs)
//   - 4 corner tiles: live wattage (icon + big number) plus a small daily
//     total tucked into the unused space above/below each icon
//   - center: clock, grid status/WiFi, today's cost or savings, and EV/A-C
//     load badges (only colored up when actually drawing power)
//
// All sensor state is pulled straight from id(...) so the YAML lambda call
// stays a one-liner: draw_all(it, font_64, font_40, font_20, icon_96, icon_40, 800, 800);

// ---------- small helpers ----------

// Formats a power/energy value with no more than 1 decimal, and hides -0.0.
void format_number(esphome::display::Display& it,
                    esphome::display::BaseFont& font,
                    esphome::Color& color,
                    esphome::display::TextAlign align,
                    int x, int y,
                    float value) {
    if (value < .1 && value > -.1) {
        value = 0;
    }
    if (value >= 10 || value <= -10 || value == 0) {
        it.printf(x, y, &font, color, align, "%.0f", value);
    } else {
        it.printf(x, y, &font, color, align, "%.1f", value);
    }
}

// Draws one 5-degree-stepped filled arc segment from start->end degrees
// (0 = top, clockwise), out to radius width/2 + radius_offset.
void draw_arc(esphome::display::Display& it,
              int start,
              int end,
              esphome::Color& color,
              int width, int height,
              int radius_offset = 20) {
    start = start - 90;
    end = end - 90;
    int l = width / 2 + radius_offset;
    int step = (end >= start) ? 5 : -5;
    int a = start;
    while ((step > 0 && a < end) || (step < 0 && a > end)) {
        int b = a + step;
        if ((step > 0 && b > end) || (step < 0 && b < end)) {
            b = end;
        }
        int x1 = width / 2 + (l * cos(a * 0.0174533));
        int y1 = height / 2 + (l * sin(a * 0.0174533));
        int x2 = width / 2 + (l * cos(b * 0.0174533));
        int y2 = height / 2 + (l * sin(b * 0.0174533));
        it.filled_triangle(width / 2, height / 2, x1, y1, x2, y2, color);
        a = b;
    }
}

// Draws the colored "fill" of a gauge segment sized by current vs min/max,
// on top of the dim background track. Up (positive) fills clockwise from
// the middle of the quadrant, down (negative) fills counter-clockwise.
void draw_power_gauge(esphome::display::Display& it,
                       float min_val,
                       float max_val,
                       float current,
                       esphome::Color& pos_color,
                       esphome::Color& neg_color,
                       int start,
                       int width, int height) {
    if (current == 0) return;
    esphome::Color color = pos_color;
    int sweep = 0;
    if (current < 0) {
        color = neg_color;
        sweep = (current / min_val) * 90;
        if (sweep > 90) sweep = 90;
    } else {
        sweep = (current / max_val) * 90;
        if (sweep > 90) sweep = 90;
    }
    if (((start < 180) && (current > 0)) || ((start >= 180) && (current < 0))) {
        int end = start + 90;
        start = start + 90 - sweep;
        draw_arc(it, start, end, color, width, height);
    } else {
        int end = start + sweep;
        draw_arc(it, start, end, color, width, height);
    }
}

void draw_battery_icon(esphome::display::Display& it,
                        float charge,
                        float power,
                        int x, int y,
                        esphome::display::BaseFont& font,
                        esphome::Color& normal_color,
                        esphome::Color& low_color) {
    if (power <= -.1) {
        if (charge < 10)       it.print(x, y, &font, low_color, TextAlign::CENTER, "󰢟");
        else if (charge < 20)  it.print(x, y, &font, low_color, TextAlign::CENTER, "󰢜");
        else if (charge < 30)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂆");
        else if (charge < 40)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂇");
        else if (charge < 50)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂈");
        else if (charge < 60)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰢝");
        else if (charge < 70)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂉");
        else if (charge < 80)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰢞");
        else if (charge < 90)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂊");
        else if (charge < 100) it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂋");
        else                   it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂅");
    } else {
        if (charge < 10)       it.print(x, y, &font, low_color, TextAlign::CENTER, "󰁹");
        else if (charge < 20)  it.print(x, y, &font, low_color, TextAlign::CENTER, "󰁻");
        else if (charge < 30)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰁼");
        else if (charge < 40)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰁽");
        else if (charge < 50)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰁾");
        else if (charge < 60)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰁿");
        else if (charge < 70)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂀");
        else if (charge < 80)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂁");
        else if (charge < 90)  it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂂");
        else if (charge < 100) it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰂊");
        else                   it.print(x, y, &font, normal_color, TextAlign::CENTER, "󰁹");
    }
}

// WiFi bars from dBm, reusing the strength glyphs already in the icon font.
void draw_wifi_icon(esphome::display::Display& it,
                     float dbm,
                     int x, int y,
                     esphome::display::BaseFont& font,
                     esphome::Color& color) {
    const char* glyph;
    if (dbm >= -55)      glyph = "󰤨"; // strength-4
    else if (dbm >= -65)  glyph = "󰤥"; // strength-3
    else if (dbm >= -75)  glyph = "󰤢"; // strength-2
    else if (dbm >= -85)  glyph = "󰤟"; // strength-1
    else                   glyph = "󰤮"; // off
    it.print(x, y, &font, color, TextAlign::CENTER, glyph);
}

// Icon + big number + small unit label.
void draw_stat_tile(esphome::display::Display& it,
                     esphome::display::BaseFont& big_font,
                     esphome::display::BaseFont& small_font,
                     esphome::display::BaseFont& icon_font,
                     int icon_x, int icon_y,
                     int num_x, int num_y,
                     const char* icon,
                     float value,
                     const char* unit,
                     esphome::Color& icon_color,
                     esphome::Color& value_color) {
    it.print(icon_x, icon_y, &icon_font, icon_color, TextAlign::CENTER, icon);
    format_number(it, big_font, value_color, TextAlign::CENTER, num_x, num_y, value);
    it.print(num_x, num_y + 32, &small_font, value_color, TextAlign::CENTER, unit);
}

// ---------- daily energy buffers ----------
// The Home Assistant energy/cost sensors are cumulative (lifetime) totals,
// so the ESP itself keeps a 30-day rolling buffer of "value recorded at
// local midnight" per metric (persisted to flash via the globals'
// restore_value). buffer[0] is always today's baseline, buffer[1]
// yesterday's, and so on. "Today" = current_total - buffer[0].
//
// This has to degrade gracefully: at first boot, after a firmware update,
// or whenever the upstream HA sensor hasn't reported yet, current_total
// arrives as NaN and/or buffer[0] is still the -1 "unset" sentinel - in
// both cases we report NAN so the display can show a pending state instead
// of a bogus/negative number.

bool energy_value_ready(float v) {
    return !isnan(v) && v >= 0;
}

int days_in_year(int y) {
    bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    return leap ? 366 : 365;
}

void shift_energy_buffer(float buf[30], int days_gap) {
    if (days_gap >= 30) {
        for (int i = 0; i < 30; i++) buf[i] = -1.0f;
        return;
    }
    for (int i = 29; i >= days_gap; i--) buf[i] = buf[i - days_gap];
    for (int i = 0; i < days_gap; i++) buf[i] = -1.0f;
}

// Fills in today's baseline the first time a valid reading shows up,
// whether that's right at the midnight rollover or hours later because the
// HA sensor was briefly unavailable.
void seed_energy_buffer(float buf[30], float current) {
    if (buf[0] < -0.5f && energy_value_ready(current)) {
        buf[0] = current;
    }
}

// Called every display refresh. Cheap (a couple of int comparisons) except
// on the rare tick where the local calendar day actually changes.
void update_energy_buffers() {
    auto now = id(my_time).now();
    if (!now.is_valid()) return; // no time sync yet - skip, try again next tick

    int cur_year = now.year;
    int cur_doy = now.day_of_year;
    bool day_changed = (id(energy_last_year) != cur_year) || (id(energy_last_doy) != cur_doy);

    if (day_changed) {
        int days_gap = 1;
        if (id(energy_last_year) != -1) {
            if (cur_year == id(energy_last_year)) {
                days_gap = cur_doy - id(energy_last_doy);
            } else {
                days_gap = (days_in_year(id(energy_last_year)) - id(energy_last_doy)) + cur_doy;
            }
            if (days_gap < 1) days_gap = 1;
            if (days_gap > 30) days_gap = 30;
        }
        shift_energy_buffer(id(solar_energy_buffer), days_gap);
        shift_energy_buffer(id(home_energy_buffer), days_gap);
        shift_energy_buffer(id(grid_import_energy_buffer), days_gap);
        shift_energy_buffer(id(grid_export_energy_buffer), days_gap);
        shift_energy_buffer(id(cost_energy_buffer), days_gap);
        id(energy_last_year) = cur_year;
        id(energy_last_doy) = cur_doy;
    }

    seed_energy_buffer(id(solar_energy_buffer), id(solar_energy_total).state);
    seed_energy_buffer(id(home_energy_buffer), id(home_energy_total).state);
    seed_energy_buffer(id(grid_import_energy_buffer), id(grid_import_total).state);
    seed_energy_buffer(id(grid_export_energy_buffer), id(grid_export_total).state);
    seed_energy_buffer(id(cost_energy_buffer), id(cost_total).state);
}

// NAN = not available yet (render as pending). A negative delta means the
// upstream lifetime counter reset/rolled over - clamp to 0 rather than
// showing a nonsense negative "today" value.
float energy_today(float current, float baseline) {
    if (!energy_value_ready(current)) return NAN;
    if (baseline < -0.5f) return NAN;
    float delta = current - baseline;
    if (delta < 0) return 0.0f;
    return delta;
}

// ---------- entry point, called from the display lambda ----------

void draw_all(esphome::display::Display& it,
              esphome::display::BaseFont& big_font,    // font_64  - quadrant live numbers
              esphome::display::BaseFont& clock_font,  // font_40  - center clock
              esphome::display::BaseFont& small_font,  // font_20  - all captions
              esphome::display::BaseFont& icon_font,   // icon_96  - quadrant icons
              esphome::display::BaseFont& icon_font_sm,// icon_40  - wifi icon
              int width, int height) {
    auto black = Color(0, 0, 0);
    auto white = Color(255, 255, 255);
    auto dim = Color(75, 78, 92);
    auto track = Color(32, 34, 42);
    auto red = Color(255, 70, 70);
    auto green = Color(60, 220, 130);
    auto blue = Color(80, 150, 255);
    auto yellow = Color(255, 205, 40);
    auto orange = Color(255, 150, 40);

    int cx = width / 2, cy = height / 2;

    update_energy_buffers();

    // Flash the outage color roughly every other display refresh.
    if (!id(grid_status).state) {
        id(flash) = !id(flash);
    } else {
        id(flash) = true;
    }

    // background gauge tracks (dim), one 90-degree quadrant each
    draw_arc(it, 0, 90, track, width, height);
    draw_arc(it, 90, 180, track, width, height);
    draw_arc(it, 180, 270, track, width, height);
    draw_arc(it, 270, 360, track, width, height);

    // colored fill on top, sized by current power
    draw_power_gauge(it, -17, 20, id(grid_power).state, red, green, 0, width, height);
    draw_power_gauge(it, -10, 10, id(battery_power).state, red, blue, 90, width, height);
    draw_power_gauge(it, -100, 11, id(solar_power).state, yellow, blue, 180, width, height);
    draw_power_gauge(it, -100, 20, id(home_power).state, red, blue, 270, width, height);

    // inner disc
    it.filled_circle(cx, cy, width / 2 - 40, black);
    if (id(flash) && !id(grid_status).state) {
        it.filled_circle(cx, cy, width / 2 - 40, Color(90, 0, 0));
    }

    int tl_x = int(width * 0.3125);
    int tr_x = int(width - width * 0.3125);
    int top_icon_y = int(width * 0.229166667);
    int top_num_y = int(width * 0.395833333);
    int bot_icon_y = int(width - width * 0.229166667);
    int bot_num_y = int(width - width * 0.395833333);
    // Small caption rows tucked into the otherwise-empty space between each
    // icon and the outer gauge ring.
    int top_cap_y = int(height * 0.14);
    int bot_cap_y1 = int(height * 0.845);
    int bot_cap_y2 = int(height * 0.90);

    // ---- Home (top-left): live watts + today's kWh ----
    float home = id(home_power).state;
    esphome::Color home_c = (home >= 0) ? white : green;
    draw_stat_tile(it, big_font, small_font, icon_font, tl_x, top_icon_y, tl_x, top_num_y,
                    "󰋜", home, "W", white, home_c);
    float home_kwh = energy_today(id(home_energy_total).state, id(home_energy_buffer)[0]);
    if (isnan(home_kwh)) {
        it.print(tl_x, top_cap_y, &small_font, dim, TextAlign::CENTER, "-- kWh");
    } else {
        it.printf(tl_x, top_cap_y, &small_font, dim, TextAlign::CENTER, "%.1f kWh", home_kwh);
    }

    // ---- Grid (top-right): live watts + today's net kWh (import - export) ----
    float grid = id(grid_power).state;
    esphome::Color grid_c = (grid >= 0) ? red : green;
    draw_stat_tile(it, big_font, small_font, icon_font, tr_x, top_icon_y, tr_x, top_num_y,
                    "󰴾", grid, "W", white, grid_c);
    float grid_in = energy_today(id(grid_import_total).state, id(grid_import_energy_buffer)[0]);
    float grid_out = energy_today(id(grid_export_total).state, id(grid_export_energy_buffer)[0]);
    if (isnan(grid_in) || isnan(grid_out)) {
        it.print(tr_x, top_cap_y, &small_font, dim, TextAlign::CENTER, "-- kWh net");
    } else {
        float net = grid_in - grid_out;
        esphome::Color net_c = (net <= 0) ? green : dim;
        it.printf(tr_x, top_cap_y, &small_font, net_c, TextAlign::CENTER, "%+.1f kWh net", net);
    }

    // ---- Solar (bottom-left): live watts + today's kWh + radiation ----
    float solar = id(solar_power).state;
    draw_stat_tile(it, big_font, small_font, icon_font, tl_x, bot_icon_y, tl_x, bot_num_y,
                    "󰶜", solar, "W", white, white);
    float solar_kwh = energy_today(id(solar_energy_total).state, id(solar_energy_buffer)[0]);
    float radiation = id(solar_radiation).state;
    // Nudged a bit toward center (vs. tl_x) so this longer combined line
    // has room to clear the round edge of the display.
    int solar_cap_x = tl_x + 25;
    bool have_kwh = !isnan(solar_kwh);
    bool have_rad = !isnan(radiation);
    if (have_kwh && have_rad) {
        it.printf(solar_cap_x, bot_cap_y1, &small_font, dim, TextAlign::CENTER, "%.0f kWh  %.0f W/m2", solar_kwh, radiation);
    } else if (have_kwh) {
        it.printf(solar_cap_x, bot_cap_y1, &small_font, dim, TextAlign::CENTER, "%.0f kWh  -- W/m2", solar_kwh);
    } else if (have_rad) {
        it.printf(solar_cap_x, bot_cap_y1, &small_font, dim, TextAlign::CENTER, "-- kWh  %.0f W/m2", radiation);
    } else {
        it.print(solar_cap_x, bot_cap_y1, &small_font, dim, TextAlign::CENTER, "-- kWh  -- W/m2");
    }

    // ---- Battery (bottom-right): live watts + charge % + sunrise/sunset ----
    float batt = id(battery_power).state;
    float charge = id(battery_charge).state;
    esphome::Color batt_c = (batt < -0.1) ? blue : white;
    draw_battery_icon(it, charge, batt, tr_x, bot_icon_y, icon_font, white, red);
    format_number(it, big_font, batt_c, TextAlign::CENTER, tr_x, bot_num_y, batt);
    it.printf(tr_x, bot_num_y + 32, &small_font, batt_c, TextAlign::CENTER, "%.0f%%", charge);
    if (!id(sun_rise).state.empty()) {
        it.printf(tr_x, bot_cap_y1, &small_font, dim, TextAlign::CENTER, "R %s", id(sun_rise).state.c_str());
        it.printf(tr_x, bot_cap_y2, &small_font, dim, TextAlign::CENTER, "S %s", id(sun_set).state.c_str());
    } else {
        it.print(tr_x, bot_cap_y1, &small_font, dim, TextAlign::CENTER, "SUN --");
    }

    // ---- Center: clock, status, cost, loads ----
    auto now = id(my_time).now();
    if (now.is_valid()) {
        it.strftime(cx, cy - 45, &clock_font, white, TextAlign::CENTER, "%H:%M", now);
        it.strftime(cx, cy - 16, &small_font, dim, TextAlign::CENTER, "%a %b %d", now);
    } else {
        it.print(cx, cy - 45, &clock_font, dim, TextAlign::CENTER, "--:--");
    }

    bool gstatus = id(grid_status).state;
    esphome::Color status_c = gstatus ? green : (id(flash) ? red : dim);
    draw_wifi_icon(it, id(wifi_signal_db).state, cx - 60, cy + 8, icon_font_sm, dim);
    it.print(cx + 15, cy + 8, &small_font, status_c, TextAlign::CENTER, gstatus ? "GRID OK" : "GRID DOWN");

    float cost = energy_today(id(cost_total).state, id(cost_energy_buffer)[0]);
    if (isnan(cost)) {
        it.print(cx, cy + 30, &small_font, dim, TextAlign::CENTER, "COST PENDING");
    } else {
        bool saved = cost <= 0;
        esphome::Color cost_c = saved ? green : red;
        it.printf(cx, cy + 30, &small_font, cost_c, TextAlign::CENTER,
                  saved ? "+$%.2f today" : "-$%.2f today", fabs(cost));
    }

    float ev = id(ev_charger_power).state;
    float ac = id(ac_power).state;
    esphome::Color ev_c = (ev > 10) ? orange : dim;
    esphome::Color ac_c = (ac > 10) ? blue : dim;
    if (ev > 10) {
        it.printf(cx - 70, cy + 54, &small_font, ev_c, TextAlign::CENTER, "EV %.0fW", ev);
    } else {
        it.print(cx - 70, cy + 54, &small_font, ev_c, TextAlign::CENTER, "EV --");
    }
    if (ac > 10) {
        it.printf(cx + 70, cy + 54, &small_font, ac_c, TextAlign::CENTER, "A/C %.0fW", ac);
    } else {
        it.print(cx + 70, cy + 54, &small_font, ac_c, TextAlign::CENTER, "A/C --");
    }
}
