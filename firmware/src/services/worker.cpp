#include "worker.h"

#include <Arduino.h>
#include <task.h>
#include <queue.h>

#include "app/assets/concept_demo.h"
#include "app/canvas.h"
#include "app/epaper_selftest.h"
#include "app/widgets.h"
#include "datapipe.h"
#include "drivers/epaper.h"
#include "ntp.h"
#include "ota.h"
#include "storage.h"

namespace worker {

namespace {

enum Job : uint8_t { JOB_RENDER, JOB_PIPE, JOB_OTA };
struct Request {
  Job job;
  Render render;
  char url[512];
  char md5[33];
};

QueueHandle_t sQueue = nullptr;
volatile bool sRunning = false;

bool occupied() { return sRunning || uxQueueMessagesWaiting(sQueue) > 0; }

bool post(const Request& r) {
  return xQueueSend(sQueue, &r, 0) == pdTRUE;
}

void doRender(Render what) {
  switch (what) {
    case Render::Test:
      epaper_selftest::run();
      return;
    case Render::Demo:
      concept_demo::show();
      return;
    case Render::Bitmap:  // B3 位图通道：平面已就绪，只推屏
      epaper::init();
      canvas::flush();
      return;
    case Render::Layout: {
      // 等对时再渲染（时钟 widget 依赖真实时间；不等则上电首屏画 1970-01-01）。
      // 上限 45s：Wi-Fi/NTP 不就绪时降级照画，后续整点流水会重刷正确时间
      {
        const uint32_t tWait = millis();
        while (!ntp::synced() && millis() - tWait < 45000) delay(500);
        if (!ntp::synced())
          Serial.println("[工作] NTP 45s 未同步（Wi-Fi 未就绪），首屏时钟将显示未同步时间");
      }
      // C2：/layout.json → Widget 引擎实时渲染；无文件或解析失败回退示例帧。
      // epaper::init 不能省：RST 脉冲清面板跨会话残留态（漏调时命令被吞、屏不更新，实测）
      epaper::init();
      const String json = storage::exists("/layout.json")
                              ? storage::readFile("/layout.json") : String("");
      if (json.length() && widgets::render(json)) {
        canvas::flush();
        return;
      }
      Serial.println("[工作] 布局渲染回退：无 /layout.json 或解析失败，走内置示例帧");
      concept_demo::show();
      return;
    }
  }
}

void workerTask(void*) {
  Request r;
  for (;;) {
    xQueueReceive(sQueue, &r, portMAX_DELAY);
    sRunning = true;
    const uint32_t t0 = millis();
    switch (r.job) {
      case JOB_RENDER: doRender(r.render); break;
      case JOB_PIPE: datapipe::runOnce("队列"); break;
      case JOB_OTA: ota::fromUrl(r.url, r.md5[0] ? r.md5 : nullptr); break;
    }
    Serial.printf("[工作] 任务完成（耗时 %.1fs）\n", (millis() - t0) / 1000.0);
    sRunning = false;
  }
}

}  // namespace

void begin() {
  sQueue = xQueueCreate(2, sizeof(Request));  // 深度 2：忙碌拒绝 + 单排队
  xTaskCreatePinnedToCore(workerTask, "worker", 8192, nullptr, 1, nullptr, 0);
}

bool busy() { return occupied(); }

void requestRenderFromWorker(Render what) {  // 已在 worker 内（如 JOB_PIPE 尾），直接入队
  Request r{};
  r.job = JOB_RENDER;
  r.render = what;
  post(r);
}

bool requestRender(Render what) {
  if (occupied()) {
    Serial.println("[工作] 忙碌：渲染请求被拒（当前任务完成后可重试）");
    return false;
  }
  Request r{};
  r.job = JOB_RENDER;
  r.render = what;
  return post(r);
}

bool requestPipe() {
  if (occupied()) {
    Serial.println("[工作] 忙碌：流水请求被拒");
    return false;
  }
  Request r{};
  r.job = JOB_PIPE;
  return post(r);
}

bool requestOta(const char* url, const char* md5) {
  if (occupied()) {
    Serial.println("[工作] 忙碌：OTA 请求被拒");
    return false;
  }
  Request r{};
  r.job = JOB_OTA;
  strncpy(r.url, url, sizeof(r.url) - 1);
  if (md5) strncpy(r.md5, md5, sizeof(r.md5) - 1);
  return post(r);
}

}  // namespace worker
