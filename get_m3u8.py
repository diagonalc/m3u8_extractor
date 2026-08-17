# import sys
# import urllib.parse
# from playwright.sync_api import sync_playwright

# def get_m3u8(raw_url):
#     # 自动解码 URL (%3A -> :, %2F -> / 等)
#     target_url = urllib.parse.unquote(raw_url)
#     m3u8_url = ""

#     with sync_playwright() as p:
#         browser = p.chromium.launch(
#             headless=True,
#             args=['--disable-blink-features=AutomationControlled']
#         )
#         context = browser.new_context(
#             user_agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
#         )
#         page = context.new_page()

#         def handle_response(response):
#             nonlocal m3u8_url
#             if response.request.resource_type in ["fetch", "xhr"]:
#                 try:
#                     if "#EXTM3U" in response.text():
#                         m3u8_url = response.url
#                 except Exception:
#                     pass

#         page.on("response", handle_response)
        
#         try:
#             page.goto(target_url, wait_until="domcontentloaded", timeout=15000)
            
#             # 处理滑块弹窗
#             thumb = page.locator(".sliderCaptcha_thumb").first
#             if thumb.is_visible(timeout=3000):
#                 box = thumb.bounding_box()
#                 if box:
#                     page.mouse.move(box["x"] + 10, box["y"] + 10)
#                     page.mouse.down()
#                     page.mouse.move(box["x"] + 290, box["y"] + 10)
#                     page.mouse.up()

#             page.wait_for_timeout(4000)
#         except Exception:
#             pass
#         finally:
#             browser.close()

#     return m3u8_url

# if __name__ == "__main__":
#     if len(sys.argv) > 1:
#         # 仅将结果输出至 stdout
#         print(get_m3u8(sys.argv[1]))
import sys
import random
import urllib.parse
from playwright.sync_api import sync_playwright

def solve_slider_captcha_fast(page):
    thumb_selector = ".sliderCaptcha_thumb"
    
    try:
        page.wait_for_selector(thumb_selector, state="visible", timeout=6000)
    except Exception:
        print("[Python Debug] 未检测到滑块，可能已直接通过。", file=sys.stderr)
        return

    # 1. 计算滑动总距离
    drag_distance = page.evaluate("""() => {
        const thumb = document.querySelector('.sliderCaptcha_thumb');
        const container = thumb.parentElement;
        return (container.clientWidth - thumb.clientWidth) + 5;
    }""")

    thumb = page.locator(thumb_selector).first
    box = thumb.bounding_box()
    if not box:
        return

    start_x = box["x"] + box["width"] / 2
    start_y = box["y"] + box["height"] / 2

    # 2. 按住滑块
    page.mouse.move(start_x, start_y)
    page.mouse.down()

    # 3. 极速滑动：仅用 8 步，每步间隔 1~3ms
    steps = 8
    for i in range(1, steps + 1):
        progress = i / steps
        move_x = start_x + (drag_distance * progress)
        move_y = start_y + random.choice([0, 1])
        
        page.mouse.move(move_x, move_y)
        page.wait_for_timeout(random.randint(1, 3))  # 极短延迟

    # 4. 松开鼠标
    page.mouse.up()


def run(target_url):
    m3u8_url = ""
    with sync_playwright() as p:
        browser = p.chromium.launch(
            headless=False,  # 与 test1.py 一致：目标站点会检测无头模式，无头时抓不到 m3u8
            args=['--disable-blink-features=AutomationControlled']
        )
        context = browser.new_context(
            user_agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
        )
        page = context.new_page()

        def handle_response(response):
            nonlocal m3u8_url
            if response.request.resource_type in ["fetch", "xhr"]:
                try:
                    if "#EXTM3U" in response.text():
                        m3u8_url = response.url
                        # 日志打到 stderr，避免影响 stdout 读取
                        print(f"[Python Debug] 捕获到 M3U8: {m3u8_url}", file=sys.stderr)
                except Exception:
                    pass

        page.on("response", handle_response)

        try:
            page.goto(target_url, wait_until="domcontentloaded")

            # 执行极速滑动
            solve_slider_captcha_fast(page)

            page.wait_for_timeout(8000)
        except Exception as e:
            print(f"[Python Debug Error] {e}", file=sys.stderr)
        finally:
            browser.close()

    return m3u8_url


if __name__ == "__main__":
    # 默认 fallback 测试 URL
    target_url = "https://loli32.top/index.php/vod/play/id/987/sid/1/nid/1.html"

    # 如果 C 语言 popen 传入了参数（sys.argv[1]）
    if len(sys.argv) > 1:
        raw_input = sys.argv[1].strip()
        
        # 1. 剥离表单提交的 url= 前缀
        if "=" in raw_input and not raw_input.startswith("http"):
            raw_input = raw_input.split("=", 1)[1]
            
        # 2. 进行 URL 解码 (%3A -> :, %2F -> /) 并去空
        target_url = urllib.parse.unquote(raw_input).strip()

    # 执行爬取
    result = run(target_url)

    # 【最关键一步】：标准的 stdout 仅打印纯净的 URL 结果供 C 的 fgets 读取
    #print("hihiihi")
    print(result if result else "")