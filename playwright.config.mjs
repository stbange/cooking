import { defineConfig } from '@playwright/test';
import fs from 'node:fs';

const systemChrome = '/usr/bin/google-chrome-stable';
const hasSystemChrome = fs.existsSync(systemChrome);

export default defineConfig({
  testDir: './tests/web',
  timeout: 45000,
  use: {
    baseURL: 'http://127.0.0.1:8123',
    headless: true,
    launchOptions: {
      ...(hasSystemChrome ? { executablePath: systemChrome } : {}),
      args: [
        '--no-sandbox',
        '--use-gl=angle',
        '--use-angle=swiftshader',
        '--enable-webgl',
        '--ignore-gpu-blocklist'
      ]
    }
  },
  webServer: {
    command: 'python3 -m http.server 8123 --directory build-web --bind 127.0.0.1',
    url: 'http://127.0.0.1:8123/index.html',
    reuseExistingServer: !process.env.CI,
    timeout: 15000
  }
});
