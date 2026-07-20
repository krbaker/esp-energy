

void draw_power(esphome::display::Display& it,
                    esphome::display::BaseFont& font,
                    esphome::Color& color,
                    esphome::display::TextAlign align,
                    int x, int y, 
                    float value){
    // Remove -0.0
    if (value < .1 && value > -.1){
        value = 0;
    }
    // If > 10 drop decimal
    if (value >= 10 || value == 0){
        it.printf(x,y,&font,color,align,"%.0f", value);
    } else {
        it.printf(x,y,&font,color,align,"%.1f", value);
    }
}

void draw_arc(esphome::display::Display& it,
              int start,
              int end,
              esphome::Color& color,
              int width, int height){
    start = start - 90;
    end = end - 90;
    int l = width/2 + 20;
    int start_angle = start;
    while(start_angle < end){
        int end_angle = start_angle + 10;
        if (end_angle > end){
            end_angle = end;
        }
        int x1 = width/2 + (l * cos(start_angle * 0.0174533));
        int y1 = height/2 + (l * sin(start_angle * 0.0174533));
        int x2 = width/2 + (l * cos(end_angle * 0.0174533));
        int y2 = height/2 + (l * sin(end_angle * 0.0174533));
        it.filled_triangle(width/2,height/2, x1, y1, x2, y2, color);
        start_angle = end_angle;
    }
}

// Up is positive 'draw'
void draw_power(esphome::display::Display& it,
    float min,
    float max,
    float current,
    esphome::Color& pos_color,
    esphome::Color& neg_color,
    int start,
    int width, int height){
    if (current != 0){
        esphome::Color color = pos_color;
        int sweep = 0;
        if (current < 0){
            color = neg_color;
            sweep = (current / min) * 90;
            if (sweep > 90){
                sweep = 90;
            }
        }
        if (current > 0){
            sweep = (current / max) * 90;
            if (sweep > 90){
                sweep = 90;
            }
        }
        if (((start < 180) && (current > 0)) ||
        ((start >= 180) && (current < 0))) {
            int end = start + 90;
            start = start + 90 - sweep;
            draw_arc(it, start, end, color, width, height);
        } else {
            int end = start + sweep;
            draw_arc(it, start, end, color, width, height);
        }
    }
}


void draw_battery(esphome::display::Display& it,
    float charge,
    float power,
    int x,
    int y,
    esphome::display::BaseFont& font,
    esphome::Color& normal_color,
    esphome::Color& low_color){
    if (power <= -.1){
      if(charge < 10){
        it.print(x,y, &font, low_color, TextAlign::CENTER, "󰢟");
      } else if (charge < 20){
        it.print(x,y, &font, low_color, TextAlign::CENTER, "󰢜");
      } else if (charge < 30){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂆");
      } else if (charge < 40){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂇");
      } else if (charge < 50){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂈");
      } else if (charge < 60){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰢝");
      } else if (charge < 70){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂉");
      } else if (charge < 80){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰢞");
      } else if (charge < 90){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂊");
      } else if (charge < 100){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂋");
      } else {
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂄");
      }
    } else {  // not charging
      if(charge < 10){
        it.print(x,y, &font, low_color, TextAlign::CENTER, "󰁹");
      } else if (charge < 20){
        it.print(x,y, &font, low_color, TextAlign::CENTER, "󰁻");
      } else if (charge < 30){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰁼");
      } else if (charge < 40){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰁽");
      } else if (charge < 50){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰁾");
      } else if (charge < 60){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰁿");
      } else if (charge < 70){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂀");
      } else if (charge < 80){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂁");
      } else if (charge < 90){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂊");
      } else if (charge < 100){
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰂂");
      } else {
        it.print(x,y, &font, normal_color, TextAlign::CENTER, "󰁹");
      }
    }

}


void draw_all(esphome::display::Display& it,
              esphome::display::BaseFont& base_font,
              esphome::display::BaseFont& icon_font,
              int width, int height,
              float grid_power,
              float battery_power,
              float solar_power,
              float home_power,
              float battery_charge,
              bool grid_status,
              bool flash){
    auto black = Color(0, 0, 0);
    auto white = Color(255, 255, 255);
    auto red = Color(255, 0, 0);
    auto blue = Color(0, 0, 255);
    auto green = Color(0, 255, 0);
    auto yellow = Color(255, 255, 0);
    draw_power(it, -17, 20, grid_power, red, green, 0, width, height); 
    draw_power(it, -10, 10, battery_power, red, blue, 90, width, height); 
    draw_power(it, -100, 11, solar_power, yellow, blue, 180, width, height); 
    draw_power(it, -100, 20, home_power, red, blue, 270, width, height); 
    it.filled_circle(width/2,height/2,width/2 - 40, black);
    if (flash && !grid_status) { it.filled_circle(width/2, height/2, width/2 - 40, red); }
    it.print(int(width * 0.3125), int(width * 0.229166667), &icon_font, white, TextAlign::CENTER, "󰋜");
    draw_power(it, base_font, white, TextAlign::CENTER, int(width * 0.3125), int(width * 0.395833333), home_power);
    it.print(int(width - width * 0.3125), int(width * 0.229166667), &icon_font, white, TextAlign::CENTER, "󰴾");
    draw_power(it, base_font, white, TextAlign::CENTER, int(width - width * 0.3125), int(width * 0.395833333), grid_power);
    it.print(int(width * 0.3125), int(width - width * 0.229166667), &icon_font, white, TextAlign::CENTER, "󰶜");
    draw_power(it, base_font, white, TextAlign::CENTER, int(width * 0.3125), int(width - width * 0.395833333), solar_power);
    draw_battery(it, battery_charge, battery_power, int(width - width * 0.3125), int(width - width * 0.229166667), icon_font, white, red);
    draw_power(it, base_font, white, TextAlign::CENTER, int(width - width * 0.3125), int(width - width * 0.395833333), battery_power);
}
