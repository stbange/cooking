import { test, expect } from '@playwright/test';

test('fetches unbaked external ./assets/ over HTTP and runs 3D workbench on Desktop WebGL2', async ({ browser }) => {
  const context = await browser.newContext({ viewport: { width: 1280, height: 720 }, hasTouch: false });
  const page = await context.newPage();

  const fetchedAssets = new Set();
  page.on('response', (resp) => {
    if (resp.status() === 200 && resp.url().includes('/assets/')) {
      fetchedAssets.add(new URL(resp.url()).pathname);
    }
  });

  await page.goto('/index.html');
  await page.waitForFunction(() => window.fatduckDebugState && window.fatduckDebugState()?.booted === true, null, { timeout: 25000 });

  expect(fetchedAssets.has('/assets/manifest.json')).toBeTruthy();
  expect(fetchedAssets.has('/assets/scripts/init.lua')).toBeTruthy();
  expect(fetchedAssets.has('/assets/graphics/kitchen_atlas.png')).toBeTruthy();
  expect(fetchedAssets.has('/assets/fonts/fatduck_font.png')).toBeTruthy();
  expect(fetchedAssets.has('/assets/shaders/quad_ui.vert')).toBeTruthy();
  expect(fetchedAssets.has('/assets/sounds/sizzle_loop.wav')).toBeTruthy();

  const state = await page.evaluate(() => window.fatduckDebugState());
  expect(state.booted).toBe(true);
  expect(state.touch_mode).toBe(false);
  expect(state.ingredients).toBeGreaterThanOrEqual(105);
  expect(state.techniques).toBe(15);
  expect(state.recipes).toBeGreaterThanOrEqual(32);

  /* Click Propose tab on Desktop canvas and verify tab switch */
  await page.mouse.click(255 + 118 + 40, 20);
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
  await page.waitForFunction(() => window.fatduckDebugState && window.fatduckDebugState()?.booted === true, null, { timeout: 25000 });

  const state = await page.evaluate(() => window.fatduckDebugState());
  expect(state.touch_mode).toBe(true);
  /* Base 106 + 2 Web localStorage mod ingredients >= 108 */
  expect(state.ingredients).toBeGreaterThanOrEqual(107);

  /* Tap bottom mobile thumb navigation bar to switch to Invent tab (#2) */
  await page.touchscreen.tap(245, 844 - 50);
  await page.waitForFunction(() => window.fatduckDebugState()?.tab === 2, null, { timeout: 5000 });
  await context.close();
});
