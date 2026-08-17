import random
from playwright.sync_api import sync_playwright

def solve_slider_captcha_fast(page):
    thumb_selector = ".sliderCaptcha_thumb"
    
    try:
        page.wait_for_selector(thumb_selector, state="visible", timeout=6000)
    except Exception:
        print("未检测到滑块，可能已直接通过。")
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
    

def run():
    m3u8_url = None
    with sync_playwright() as p:
        browser = p.chromium.launch(
            headless=False,
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
                        print(f"\n获取到 M3U8: {m3u8_url}")
                except Exception:
                    pass

        page.on("response", handle_response)

        target_url = input("URL:")
        page.goto(target_url, wait_until="domcontentloaded")

        # 执行极速滑动
        solve_slider_captcha_fast(page)

        page.wait_for_timeout(8000)
        browser.close()

    return m3u8_url

if __name__ == "__main__":
    run()