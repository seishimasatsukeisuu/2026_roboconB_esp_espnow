#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// 目標座標
int target_x = 0;
int target_y = 0;

// 受信側のMACアドレスを入れる

uint8_t receiverMac[] = {0xE4, 0x65, 0xB8, 0x7E, 0x05, 0x48};

bool esp_now_send_available = true;

void OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  esp_now_send_available = true;

  if (status == ESP_NOW_SEND_SUCCESS)
    Serial.println("送信成功");
  else
    Serial.println("送信失敗");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.print("受信: ");
  char msg[len + 1];
  memcpy(msg, incomingData, len);
  msg[len] = '\0';
  Serial.println(msg);
}

typedef struct
{
  int target_x;
  int target_y;
  float target_theta; // radで送信
} TargetData;

TargetData target;

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("My MAC = ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP初期化失敗");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("ピア追加失敗");
    return;
  }

  Serial.println("入力形式:");
  Serial.println("x y theta[deg]");
  Serial.println("例: 500 300 90");

  esp_now_register_send_cb(OnDataSend);
  esp_now_register_recv_cb(OnDataRecv); // 受信コールバック登録
}

void loop()
{
  if (Serial.available() > 0)
  {
    String s = Serial.readStringUntil('\n');

    int input_x;
    int input_y;
    float input_theta_deg;

    int result = sscanf(s.c_str(),
                        "%d %d %f",
                        &input_x,
                        &input_y,
                        &input_theta_deg);

    if (result == 3)
    {
      // 度 → ラジアン
      float theta_rad = input_theta_deg * PI / 180.0f;

      target.target_x = input_x;
      target.target_y = input_y;
      target.target_theta = theta_rad;

      Serial.printf("送信: x=%d, y=%d, theta=%.2f deg -> %.4f rad\n",
                    input_x,
                    input_y,
                    input_theta_deg,
                    theta_rad);

      esp_now_send(receiverMac,
                   (uint8_t *)&target,
                   sizeof(target));
    }
    else
    {
      Serial.println("入力形式が違います");
      Serial.println("例: 500 300 90");
    }
  }
}
