import { test, expect } from '@playwright/test';

test('fetches unbaked Noto fonts & 20 locales over HTTP and runs Nuklear 3D workbench on Desktop WebGL2', async ({ browser }) => {
  const context = await browser.newContext({ viewport: { width: 1280, height: 760 }, hasTouch: false });
  const page = await context.newPage();

  const fetchedAssets = new Set();
  page.on('response', (resp) => {
    if (resp.status() === 200 && resp.url().includes('/assets/')) {
      fetchedAssets.add(new URL(resp.url()).pathname);
    }
  });

  await page.goto('/index.html');
  await page.waitForFunction(() => window.fatduckDebugState && window.fatduckDebugState()?.booted === true, null, { timeout: 30000 });

  expect(fetchedAssets.has('/assets/manifest.json')).toBeTruthy();
  expect(fetchedAssets.has('/assets/scripts/init.lua')).toBeTruthy();
  expect(fetchedAssets.has('/assets/graphics/kitchen_atlas.png')).toBeTruthy();
  expect(fetchedAssets.has('/assets/fonts/NotoSans-UI.ttf')).toBeTruthy();
  expect(fetchedAssets.has('/assets/fonts/NotoSans-Arabic.ttf')).toBeTruthy();
  expect(fetchedAssets.has('/assets/fonts/NotoSans-CJK-SC.otf')).toBeTruthy();
  expect(fetchedAssets.has('/assets/locales/en.lua')).toBeTruthy();
  expect(fetchedAssets.has('/assets/locales/de.lua')).toBeTruthy();
  expect(fetchedAssets.has('/assets/locales/zh.lua')).toBeTruthy();
  expect(fetchedAssets.has('/assets/locales/ar.lua')).toBeTruthy();
  expect(fetchedAssets.has('/assets/shaders/quad_ui.vert')).toBeTruthy();
  expect(fetchedAssets.has('/assets/sounds/sizzle_loop.wav')).toBeTruthy();

  const state = await page.evaluate(() => window.fatduckDebugState());
  expect(state.booted).toBe(true);
  expect(state.ui).toBe('nuklear');
  expect(state.touch_mode).toBe(false);
  expect(state.lang_count).toBe(20);
  expect(state.vert_count).toBeGreaterThan(100);
  expect(state.ingredients).toBeGreaterThanOrEqual(105);
  expect(state.techniques).toBe(15);
  expect(state.recipes).toBeGreaterThanOrEqual(32);

  /* Switch languages dynamically across Latin, CJK, and RTL Arabic */
  await page.evaluate(() => window.fatduckSetLang('de'));
  await page.waitForFunction(() => window.fatduckDebugState()?.lang === 'de', null, { timeout: 5000 });
  await page.evaluate(() => window.fatduckSetLang('zh'));
  await page.waitForFunction(() => window.fatduckDebugState()?.lang === 'zh', null, { timeout: 5000 });
  await page.evaluate(() => window.fatduckSetLang('ar'));
  await page.waitForFunction(() => window.fatduckDebugState()?.lang === 'ar', null, { timeout: 5000 });
  await page.evaluate(() => window.fatduckSetLang('en'));
  await page.waitForFunction(() => window.fatduckDebugState()?.lang === 'en', null, { timeout: 5000 });

  /* Open Help via F1 and verify modal activates */
  await page.keyboard.press('F1');
  await page.waitForFunction(() => window.fatduckDebugState()?.help_open === true, null, { timeout: 5000 });
  await page.keyboard.press('F1');
  await page.waitForFunction(() => window.fatduckDebugState()?.help_open === false, null, { timeout: 5000 });

  /* Click Classics tab (#1) on Desktop Nuklear header and verify switch */
  await page.mouse.click(315, 25);
  await page.waitForFunction(() => window.fatduckDebugState()?.tab === 1, null, { timeout: 5000 });
  await context.close();
});

test('automatically switches to touch controls on mobile devices and applies localStorage Lua mods', async ({ browser }) => {
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
    hasTouch: true,
    isMobile: true
  });
  const page = await context.newPage();

  await page.addInitScript(() => {
    window.localStorage.setItem(
      'fatduck_mod:scripts/mods/example_mod.lua',
      `fatduck.add_ingredient({ id='yuzu_juice_web', name='Fresh Yuzu (Web Mod)', category='acid', water=0.89, sugar=0.03, ph=2.7, acidity_molar=0.28, diet={'vegan','gluten_free'}, roles={'acid','aromatic'} })
       fatduck.add_ingredient({ id='koji_paste_web', name='Shio Koji (Web Mod)', category='umami', water=0.55, protein=0.08, sugar=0.22, ph=5.4, sodium_mg_100g=3800, glutamate_mg_100g=550, diet={'vegan','gluten_free'}, roles={'umami','salt'} })`
    );
  });

  await page.goto('/index.html');
  await page.waitForFunction(() => window.fatduckDebugState && window.fatduckDebugState()?.booted === true, null, { timeout: 30000 });

  const state = await page.evaluate(() => window.fatduckDebugState());
  expect(state.touch_mode).toBe(true);
  expect(state.ui).toBe('nuklear');
  expect(state.ingredients).toBeGreaterThanOrEqual(107);

  /* Tap bottom mobile thumb navigation bar to switch to Invent tab (#2) */
  await page.touchscreen.tap(245, 844 - 55);
  await page.waitForFunction(() => window.fatduckDebugState()?.tab === 2, null, { timeout: 5000 });
  await context.close();
});
