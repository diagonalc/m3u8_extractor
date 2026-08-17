import sys
import urllib.parse
from playwright.sync_api import sync_playwright

def get_m3u8(raw_url):
    # 自动解码 URL (%3A -> :, %2F -> / 等)
    target_url = urllib.parse.unquote(raw_url)
    m3u8_url = ""

    with sync_playwright() as p:
        browser = p.chromium.launch(
            headless=True,
            args=['--disable-blink-features=AutomationControlled']
        )
        context = browser.new_context(
            user_agent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
        )
        page = context.new_page()

        def handle_response(response):
            nonlocal m3u8_url
            if response.request.resource_type in ["fetch", "xhr"]:
                try:
                    if "#EXTM3U" in response.text():
                        m3u8_url = response.url
                except Exception:
                    pass

        page.on("response", handle_response)
        
        try:
            page.goto(target_url, wait_until="domcontentloaded", timeout=15000)
            
            # 处理滑块弹窗
            thumb = page.locator(".sliderCaptcha_thumb").first
            if thumb.is_visible(timeout=3000):
                box = thumb.bounding_box()
                if box:
                    page.mouse.move(box["x"] + 10, box["y"] + 10)
                    page.mouse.down()
                    page.mouse.move(box["x"] + 290, box["y"] + 10)
                    page.mouse.up()

            page.wait_for_timeout(4000)
        except Exception:
            pass
        finally:
            browser.close()

    return m3u8_url

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # 仅将结果输出至 stdout
        print(get_m3u8(sys.argv[1]))