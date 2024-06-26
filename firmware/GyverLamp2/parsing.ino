char buf[UDP_TX_PACKET_MAX_SIZE + 1];

const byte sync_ver = 12;

void sync_data(int data_type) {

  char reply[100];
  mString packet(reply);
  packet.clear();
  packet = packet + "GL_RESP," + data_type + ",";
  switch (data_type){
    case 0:
      packet = packet + now.day + "," + now.hour + "," + now.min + "," + now.sec + ",";
      packet = packet + cfg.curPreset +"," + cfg.WiFimode + "," + cfg.role + "," + cfg.group;
      break;
    case 1:
      packet = packet + cfg.bright + "," + cfg.adcMode + "," + cfg.minBright + "," + cfg.maxBright + "," + cfg.rotation + "," + cfg.rotRnd + ",";
      packet = packet + cfg.rotPeriod + "," + cfg.deviceType + "," + cfg.maxCur + "," + cfg.workFrom + "," + cfg.workTo + "," + cfg.matrix + ",";
      packet = packet + cfg.length + "," + cfg.width + "," + cfg.GMT + "," + cfg.cityID + ",";
      packet = packet + cfg.mqtt + "," + cfg.mqttID + "," + cfg.mqttHost + "," + cfg.mqttPort + "," + cfg.mqttLogin + "," + cfg.mqttPass + ",";
      packet = packet + now.day + "," + now.hour + "," + now.min + "," + now.sec;
      break;
    case 2:
      packet = packet + cfg.curPreset + "," + cfg.presetAmount + "," + now.day + "," + now.hour + "," + now.min + "," + now.sec + ",";
      for (int i = 0; i < cfg.presetAmount; i++){
        packet = packet + preset[i].effect + "," + preset[i].fadeBright + "," + preset[i].bright + "," + preset[i].advMode + "," + preset[i].soundReact + "," + preset[i].min + "," + preset[i].max + "," + preset[i].speed + "," + preset[i].palette + "," + preset[i].scale + "," + preset[i].fromCenter + "," + preset[i].color + "," + preset[i].fromPal;
      }
      break;
    case 3:
      packet = packet + dawn.state[0] + "," + dawn.state[1] + "," +  dawn.state[2] + "," + dawn.state[3] + "," + dawn.state[4] + "," + dawn.state[5] + "," + dawn.state[6] + ",";
      packet = packet + dawn.hour[0] + "," + dawn.hour[1] + "," +  dawn.hour[2] + "," + dawn.hour[3] + "," + dawn.hour[4] + "," + dawn.hour[5] + "," + dawn.hour[6] + ",";
      packet = packet + dawn.minute[0] + "," + dawn.minute[1] + "," +  dawn.minute[2] + "," + dawn.minute[3] + "," + dawn.minute[4] + "," + dawn.minute[5] + "," + dawn.minute[6] + ",";
      packet = packet + dawn.bright + "," + dawn.time + "," + dawn.post + ",";
      packet = packet + now.day + "," + now.hour + "," + now.min + "," + now.sec;
      break;
    case 5:
      packet = packet + pal.size + ",";
      for (int i = 0; i < 16 * 3; i++){
        packet = packet + pal.strip[i] + ",";
      }
      packet = packet + now.day + "," + now.hour + "," + now.min + "," + now.sec;
      break;
  }

  DEBUGLN(reply);
  FOR_i(0, 4) {
    sendUDP(reply);
    delay(15);
  }

}

void parsing() {
  if (Udp.parsePacket()) {
    int n = Udp.read(buf, UDP_TX_PACKET_MAX_SIZE);
    buf[n] = NULL;

    // ПРЕ-ПАРСИНГ (для данных АЦП)
    if (buf[0] != 'G' || buf[1] != 'L' || buf[2] != ',') return;  // защита от не наших данных
    if (buf[3] == '7') {   // АЦП GL,7,
      if (!cfg.role) {     // принимаем данные ацп если слейв
        int data[3];
        mString ints(buf + 5);
        ints.parseInts(data, 3);
        udpLength = data[0];
        udpScale = data[1];
        udpBright = data[2];
        effTmr.force();   // форсируем отрисовку эффекта
        gotADCtmr = millis();
      }
      return;   // выходим
    }

    if (millis() - udpTmr < 500) return;   // принимаем остальные посылки не чаще 2 раз в секунду
    udpTmr = millis();

    DEBUGLN(buf);   // пакет вида <GL>,<тип>,<дата1>,<дата2>...

    // ПАРСИНГ
    byte data[MAX_PRESETS * PRES_SIZE + 10];
    memset(data, 0, MAX_PRESETS * PRES_SIZE + 10);
    int count = 0;
    char *str, *p = buf;
    char *ssid, *pass;

    while ((str = strtok_r(p, ",", &p)) != NULL) {
      uint32_t thisInt = atoi(str);
      data[count++] = (byte)thisInt;  // парс байтов
      // парс "тяжёлых" данных
      if (data[1] == 0) {
        if (count == 4) ssid = str;
        if (count == 5) pass = str;
      }
      if (data[1] == 1) {
        if (count == 15) cfg.length = thisInt;
        if (count == 16) cfg.width = thisInt;
        if (count == 17) cfg.GMT = byte(thisInt);
        if (count == 18) cfg.cityID = thisInt;
        if (count == 19) cfg.mqtt = byte(thisInt);
        if (count == 20) strcpy(cfg.mqttID, str);
        if (count == 21) strcpy(cfg.mqttHost, str);
        if (count == 22) cfg.mqttPort = thisInt;
        if (count == 23) strcpy(cfg.mqttLogin, str);
        if (count == 24) strcpy(cfg.mqttPass, str);
      }
    }

    // тип 0 - control, 1 - config, 2 - effects, 3 - dawn, 4 - from master, 5 - palette, 6 - time, 20 - sync
    switch (data[1]) {
      case 20: DEBUGLN("Sync"); blinkTmr.restart();
        switch (data[2]) {
          case 0:
            sync_data(0);
            DEBUGLN("Sync Data 0");
            break;
          case 1:
            sync_data(1);
            DEBUGLN("Sync Data 1");
            break;
          case 2:
            sync_data(2);
            DEBUGLN("Sync Data 2");
            break;
          case 3:
            sync_data(3);
            DEBUGLN("Sync Data 3");
            break;
          case 5:
            sync_data(5);
            DEBUGLN("Sync Data 5");
            break;
          case 6:
            sendUDP(sync_ver);
            DEBUGLN("Sync ver = " + String(sync_ver));
            break;
        }
      case 0: DEBUGLN("Control"); blinkTmr.restart();
        if (!cfg.state && data[2] != 1) return;   // если лампа выключена и это не команда на включение - не обрабатываем
        switch (data[2]) {
          case 0: controlHandler(0); break;               // выкл
          case 1: controlHandler(1); break;               // вкл
          case 2: cfg.minLight = phot.getRaw(); break;    // мин яркость
          case 3: cfg.maxLight = phot.getRaw(); break;    // макс яркость
          case 4: changePreset(-1); break;                // пред пресет
          case 5: changePreset(1); break;                 // след пресет
          case 6: setPreset(data[3] - 1); break;          // конкретный пресет data[3]
          case 7: cfg.WiFimode = data[3]; EE_updCfgRst(); break;  // смена режима WiFi
          case 8: cfg.role = data[3]; break;              // смена роли
          case 9: cfg.group = data[3]; restartUDP(); break;   // смена группы
          case 10:                                        // установка настроек WiFi
            strcpy(cfg.ssid, ssid);
            strcpy(cfg.pass, pass);
            break;
          case 11: EE_updCfgRst(); break;                 // рестарт
          case 12: if (gotNTP) {                          // OTA обновление, если есть интернет
              cfg.update = 1;
              EE_updCfg();
              FastLED.clear();
              FastLED.show();
              char OTA[60];
              mString ota(OTA);
              ota.clear();
              ota += OTAhost;
              ota += OTAfile[data[3]];
              DEBUG("Update to ");
              DEBUGLN(OTA);
              delay(100);
              WiFiClient client;
              ESPhttpUpdate.update(client, OTA);
            } break;
          case 13:                                        // выключить через
            if (data[3] == 0) turnoffTmr.stop();
            else {
              DEBUGLN("Fade");
              fadeDown((uint32_t)data[3] * 60000ul);
            }
            break;
        }
        if (data[2] < 7) setTime(data[3], data[4], data[5], data[6]);
        EE_updCfg();
        break;

      case 1: DEBUGLN("Config"); blinkTmr.restart();
        FOR_i(0, CFG_SIZE) {
          *((byte*)&cfg + i) = data[i + 2];   // загоняем в структуру
        }
        setTime(data[CFG_SIZE + 10 + 2], data[CFG_SIZE + 10 + 3], data[CFG_SIZE + 10 + 4], data[CFG_SIZE + 10 + 5]);
        if (cfg.deviceType == GL_TYPE_STRIP) {
          if (cfg.length > MAX_LEDS) cfg.length = MAX_LEDS;
          cfg.width = 1;
        }
        if (cfg.length * cfg.width > MAX_LEDS) cfg.width = MAX_LEDS / cfg.length;
        ntp.setTimeOffset((cfg.GMT - 13) * 3600);
        FastLED.setMaxPowerInVoltsAndMilliamps(STRIP_VOLT, cfg.maxCur * 100);
        if (cfg.adcMode == GL_ADC_BRI) switchToPhot();
        else if (cfg.adcMode == GL_ADC_MIC) switchToMic();
        else disableADC();
        EE_updCfg();
        break;

      case 2: DEBUGLN("Preset");
        {
          cfg.presetAmount = data[2];   // кол-во режимов
          FOR_j(0, cfg.presetAmount) {
            FOR_i(0, PRES_SIZE) {
              *((byte*)&preset + j * PRES_SIZE + i) = data[j * PRES_SIZE + i + 3]; // загоняем в структуру
            }
          }
          //if (!cfg.rotation) setPreset(data[cfg.presetAmount * PRES_SIZE + 3] - 1);
          byte dataStart = cfg.presetAmount * PRES_SIZE + 3;
          setPreset(data[dataStart] - 1);
          setTime(data[dataStart + 1], data[dataStart + 2], data[dataStart + 3], data[dataStart + 4]);

          EE_updatePreset();
          //presetRotation(true); // форсировать смену режима
          holdPresTmr.restart();
          loading = true;
        }
        break;

      case 3: DEBUGLN("Dawn"); blinkTmr.restart();
        FOR_i(0, DAWN_SIZE) {
          *((byte*)&dawn + i) = data[i + 2]; // загоняем в структуру
        }
        setTime(data[DAWN_SIZE + 2], data[DAWN_SIZE + 3], data[DAWN_SIZE + 4], data[DAWN_SIZE + 5]);
        EE_updateDawn();
        break;

      case 4: DEBUGLN("From master");
        if (cfg.role == GL_SLAVE) {
          switch (data[2]) {
            case 0: fade(data[3]); break;         // вкл выкл
            case 1: setPreset(data[3]); break;    // пресет
            case 2: cfg.bright = data[3]; break;  // яркость
          }
          EE_updateCfg();
        }
        break;

      case 5: DEBUGLN("Palette"); blinkTmr.restart();
        FOR_i(0, PAL_SIZE) {
          *((byte*)&pal + i) = data[i + 2]; // загоняем в структуру
        }
        setTime(data[PAL_SIZE + 2], data[PAL_SIZE + 3], data[PAL_SIZE + 4], data[PAL_SIZE + 5]);
        updPal();
        EE_updatePal();
        break;

      case 6: DEBUGLN("Time from AP");
        if (cfg.WiFimode && !gotNTP) {   // время для local устройств в сети AP лампы (не получили время из интернета)
          now.day = data[2];
          now.hour = data[3];
          now.min = data[4];
          now.sec = 0;
          now.setMs(0);
          DEBUGLN("Got time from master");
        }
        break;
    }
    FastLED.clear();    // на всякий случай
  }
}

void sendToSlaves(byte data1, byte data2) {
  if (cfg.role == GL_MASTER) {
    char reply[15];
    mString packet(reply);
    packet.clear();
    packet = packet + "GL,4," + data1 + ',' + data2;

    DEBUG("Sending to Slaves: ");
    DEBUGLN(reply);

    FOR_i(0, 4) {
      sendUDP(reply);
      delay(8);
    }
  }
}
