// power_display.h
// Round 800x800 power-flow dashboard for the Waveshare ESP32-P4 3.4" display.
//
// Three pages, tap the screen to cycle:
//   0 - Power Flow  : live solar/grid/battery/home wattage + clock
//   1 - Today       : daily kWh totals + energy cost/savings
//   2 - Sun & Loads : solar radiation, sunrise/sunset, EV charger, A/C
//
// All sensor state is pulled straight from id(...) so the YAML lambda call
// stays a one-liner: draw_all(it, font_64, font_28, font_20, icon_96, icon_40, 800, 800);

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

void draw_page_dots(esphome::display::Display& it,
                     int cx, int y, int count, int active,
                     esphome::Color& on, esphome::Color& off) {
    int spacing = 22;
    int start_x = cx - (spacing * (count - 1)) / 2;
    for (int i = 0; i < count; i++) {
        esphome::Color c = (i == active) ? on : off;
        it.filled_circle(start_x + i * spacing, y, 5, c);
    }
}

// Icon + big number + small unit label, used by all three pages.
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

// Same as draw_stat_tile, but shows a dimmed "--- / PENDING" placeholder
// when value is NaN instead of a misleading number.
void draw_stat_tile_or_pending(esphome::display::Display& it,
                                esphome::display::BaseFont& big_font,
                                esphome::display::BaseFont& small_font,
                                esphome::display::BaseFont& icon_font,
                                int icon_x, int icon_y,
                                int num_x, int num_y,
                                const char* icon,
                                float value,
                                const char* unit,
                                esphome::Color& icon_color,
                                esphome::Color& value_color,
                                esphome::Color& dim_color) {
    if (isnan(value)) {
        it.print(icon_x, icon_y, &icon_font, dim_color, TextAlign::CENTER, icon);
        it.print(num_x, num_y, &big_font, dim_color, TextAlign::CENTER, "--");
        it.print(num_x, num_y + 32, &small_font, dim_color, TextAlign::CENTER, "PENDING");
        return;
    }
    draw_stat_tile(it, big_font, small_font, icon_font, icon_x, icon_y, num_x, num_y,
                    icon, value, unit, icon_color, value_color);
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

// ---------- pages ----------

void draw_flow_page(esphome::display::Display& it,
                     esphome::display::BaseFont& big_font,
                     esphome::display::BaseFont& med_font,
                     esphome::display::BaseFont& small_font,
                     esphome::display::BaseFont& icon_font,
                     esphome::display::BaseFont& icon_font_sm,
                     int width, int height,
                     esphome::Color& white, esphome::Color& dim,
                     esphome::Color& red, esphome::Color& green,
                     esphome::Color& blue) {
    float grid = id(grid_power).state;
    float batt = id(battery_power).state;
    float solar = id(solar_power).state;
    float home = id(home_power).state;
    float charge = id(battery_charge).state;

    esphome::Color home_c = (home >= 0) ? white : green;
    esphome::Color grid_c = (grid >= 0) ? red : green;
    esphome::Color solar_c = white;
    esphome::Color batt_c = (batt < -0.1) ? blue : white;

    draw_stat_tile(it, big_font, small_font, icon_font,
                    int(width * 0.3125), int(width * 0.229166667),
                    int(width * 0.3125), int(width * 0.395833333),
                    "󰋜", home, "W", white, home_c);
    draw_stat_tile(it, big_font, small_font, icon_font,
                    int(width - width * 0.3125), int(width * 0.229166667),
                    int(width - width * 0.3125), int(width * 0.395833333),
                    "󰴾", grid, "W", white, grid_c);
    draw_stat_tile(it, big_font, small_font, icon_font,
                    int(width * 0.3125), int(width - width * 0.229166667),
                    int(width * 0.3125), int(width - width * 0.395833333),
                    "󰶜", solar, "W", white, solar_c);

    draw_battery_icon(it, charge, batt, int(width - width * 0.3125), int(width - width * 0.229166667),
                       icon_font, white, red);
    format_number(it, big_font, batt_c, TextAlign::CENTER,
                  int(width - width * 0.3125), int(width - width * 0.395833333), batt);
    it.printf(int(width - width * 0.3125), int(width - width * 0.395833333) + 32,
              &small_font, batt_c, TextAlign::CENTER, "W  %.0f%%", charge);

    // center clock
    int cx = width / 2, cy = height / 2;
    auto now = id(my_time).now();
    if (now.is_valid()) {
        it.strftime(cx, cy - 34, &big_font, white, TextAlign::CENTER, "%H:%M", now);
        it.strftime(cx, cy + 6, &small_font, dim, TextAlign::CENTER, "%a %b %d", now);
    } else {
        it.print(cx, cy - 34, &big_font, dim, TextAlign::CENTER, "--:--");
    }

    bool gstatus = id(grid_status).state;
    esphome::Color status_c = gstatus ? green : (id(flash) ? red : dim);
    it.print(cx, cy + 44, &small_font, status_c, TextAlign::CENTER, gstatus ? "GRID OK" : "GRID DOWN");
    draw_wifi_icon(it, id(wifi_signal_db).state, cx - 55, cy + 44, icon_font_sm, dim);
}

void draw_today_page(esphome::display::Display& it,
                      esphome::display::BaseFont& big_font,
                      esphome::display::BaseFont& med_font,
                      esphome::display::BaseFont& small_font,
                      esphome::display::BaseFont& icon_font,
                      int width, int height,
                      esphome::Color& white, esphome::Color& dim,
                      esphome::Color& red, esphome::Color& green) {
    float solar_kwh = energy_today(id(solar_energy_total).state, id(solar_energy_buffer)[0]);
    float home_kwh = energy_today(id(home_energy_total).state, id(home_energy_buffer)[0]);
    float grid_in_kwh = energy_today(id(grid_import_total).state, id(grid_import_energy_buffer)[0]);
    float grid_out_kwh = energy_today(id(grid_export_total).state, id(grid_export_energy_buffer)[0]);
    float cost = energy_today(id(cost_total).state, id(cost_energy_buffer)[0]);
    float charge = id(battery_charge).state;

    it.print(width / 2, int(height * 0.06), &small_font, dim, TextAlign::CENTER, "TODAY");

    draw_stat_tile_or_pending(it, big_font, small_font, icon_font,
                    int(width * 0.3125), int(width * 0.229166667),
                    int(width * 0.3125), int(width * 0.395833333),
                    "󰋜", home_kwh, "kWh", white, white, dim);
    draw_stat_tile_or_pending(it, big_font, small_font, icon_font,
                    int(width - width * 0.3125), int(width * 0.229166667),
                    int(width - width * 0.3125), int(width * 0.395833333),
                    "󰴾", grid_in_kwh, "kWh in", white, white, dim);
    if (isnan(grid_out_kwh)) {
        it.print(int(width - width * 0.3125), int(width * 0.395833333) + 56,
                  &small_font, dim, TextAlign::CENTER, "-- kWh out");
    } else {
        it.printf(int(width - width * 0.3125), int(width * 0.395833333) + 56,
                  &small_font, dim, TextAlign::CENTER, "%.1f kWh out", grid_out_kwh);
    }
    draw_stat_tile_or_pending(it, big_font, small_font, icon_font,
                    int(width * 0.3125), int(width - width * 0.229166667),
                    int(width * 0.3125), int(width - width * 0.395833333),
                    "󰶜", solar_kwh, "kWh", white, white, dim);

    it.print(int(width - width * 0.3125), int(width - width * 0.229166667), &icon_font, white,
              TextAlign::CENTER, "󰂅");
    if (isnan(charge)) {
        it.print(int(width - width * 0.3125), int(width - width * 0.395833333), &big_font, dim,
                  TextAlign::CENTER, "--");
    } else {
        it.printf(int(width - width * 0.3125), int(width - width * 0.395833333), &big_font, white,
                  TextAlign::CENTER, "%.0f%%", charge);
    }
    it.print(int(width - width * 0.3125), int(width - width * 0.395833333) + 32, &small_font, dim,
              TextAlign::CENTER, "CHARGE");

    // center: cost or savings
    int cx = width / 2, cy = height / 2;
    if (isnan(cost)) {
        it.print(cx, cy - 12, &big_font, dim, TextAlign::CENTER, "--");
        it.print(cx, cy + 26, &small_font, dim, TextAlign::CENTER, "COST PENDING");
    } else {
        bool saved = cost <= 0;
        esphome::Color cost_c = saved ? green : red;
        it.printf(cx, cy - 12, &big_font, cost_c, TextAlign::CENTER, "$%.2f", fabs(cost));
        it.print(cx, cy + 26, &small_font, dim, TextAlign::CENTER, saved ? "SAVED TODAY" : "COST TODAY");
    }
}

void draw_loads_page(esphome::display::Display& it,
                      esphome::display::BaseFont& big_font,
                      esphome::display::BaseFont& med_font,
                      esphome::display::BaseFont& small_font,
                      esphome::display::BaseFont& icon_font,
                      int width, int height,
                      esphome::Color& white, esphome::Color& dim,
                      esphome::Color& orange, esphome::Color& blue) {
    float ev = id(ev_charger_power).state;
    float ac = id(ac_power).state;
    float radiation = id(solar_radiation).state;

    it.print(width / 2, int(height * 0.06), &small_font, dim, TextAlign::CENTER, "SUN & LOADS");

    // EV charger
    esphome::Color ev_c = (ev > 10) ? orange : dim;
    it.print(int(width * 0.3125), int(width * 0.229166667), &icon_font, ev_c, TextAlign::CENTER, "󰭬");
    if (ev > 10) {
        format_number(it, big_font, ev_c, TextAlign::CENTER, int(width * 0.3125), int(width * 0.395833333), ev);
        it.print(int(width * 0.3125), int(width * 0.395833333) + 32, &small_font, ev_c, TextAlign::CENTER, "W EV");
    } else {
        it.print(int(width * 0.3125), int(width * 0.395833333), &med_font, dim, TextAlign::CENTER, "OFF");
        it.print(int(width * 0.3125), int(width * 0.395833333) + 32, &small_font, dim, TextAlign::CENTER, "EV CHARGER");
    }

    // A/C
    esphome::Color ac_c = (ac > 10) ? blue : dim;
    it.print(int(width - width * 0.3125), int(width * 0.229166667), &icon_font, ac_c, TextAlign::CENTER, "󰵃");
    if (ac > 10) {
        format_number(it, big_font, ac_c, TextAlign::CENTER, int(width - width * 0.3125), int(width * 0.395833333), ac);
        it.print(int(width - width * 0.3125), int(width * 0.395833333) + 32, &small_font, ac_c, TextAlign::CENTER, "W A/C");
    } else {
        it.print(int(width - width * 0.3125), int(width * 0.395833333), &med_font, dim, TextAlign::CENTER, "OFF");
        it.print(int(width - width * 0.3125), int(width * 0.395833333) + 32, &small_font, dim, TextAlign::CENTER, "A/C");
    }

    // solar radiation
    it.print(int(width * 0.3125), int(width - width * 0.229166667), &icon_font, white, TextAlign::CENTER, "󰶜");
    format_number(it, big_font, white, TextAlign::CENTER, int(width * 0.3125), int(width - width * 0.395833333), radiation);
    it.print(int(width * 0.3125), int(width - width * 0.395833333) + 32, &small_font, dim, TextAlign::CENTER, "W/m2 RAD");

    // sunrise / sunset
    int sx = int(width - width * 0.3125);
    int sy1 = int(width - width * 0.44);
    int sy2 = int(width - width * 0.36);
    if (!id(sun_rise).state.empty()) {
        it.printf(sx, sy1, &med_font, white, TextAlign::CENTER, "RISE %s", id(sun_rise).state.c_str());
        it.printf(sx, sy2, &med_font, white, TextAlign::CENTER, "SET  %s", id(sun_set).state.c_str());
    } else {
        it.print(sx, sy1, &small_font, dim, TextAlign::CENTER, "SUN DATA");
        it.print(sx, sy2, &small_font, dim, TextAlign::CENTER, "PENDING");
    }
}

// ---------- entry point, called from the display lambda ----------

void draw_all(esphome::display::Display& it,
              esphome::display::BaseFont& big_font,
              esphome::display::BaseFont& med_font,
              esphome::display::BaseFont& small_font,
              esphome::display::BaseFont& icon_font,
              esphome::display::BaseFont& icon_font_sm,
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

    int page_idx = id(page) % 3;
    if (page_idx == 0) {
        draw_flow_page(it, big_font, med_font, small_font, icon_font, icon_font_sm, width, height,
                        white, dim, red, green, blue);
    } else if (page_idx == 1) {
        draw_today_page(it, big_font, med_font, small_font, icon_font, width, height,
                         white, dim, red, green);
    } else {
        draw_loads_page(it, big_font, med_font, small_font, icon_font, width, height,
                         white, dim, orange, blue);
    }

    draw_page_dots(it, cx, height - 34, 3, page_idx, white, dim);
}
