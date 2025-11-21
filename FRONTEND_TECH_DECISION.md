# フロントエンド技術選定 決定版

**作成日**: 2025-11-21
**対象**: 外観検査アプリケーション Webフロントエンド
**ステータス**: 推奨構成確定

---

## 🎯 結論: 推奨技術スタック

### 最終推奨構成

```
フレームワーク: React 18 + TypeScript
ビルドツール: Vite
ルーティング: React Router v6
状態管理: Zustand + React Query
UIライブラリ: shadcn/ui + Tailwind CSS
グラフ: Recharts
API通信: Axios
フォーム: React Hook Form + Zod
テスト: Vitest + React Testing Library
デプロイ: Vercel
```

---

## 📊 技術選定理由（詳細）

### 1. React + TypeScript を選ぶ理由

| 要素 | React | Vue.js | 評価 |
|------|-------|--------|------|
| **エコシステム** | 非常に豊富 | 豊富 | React 勝利 |
| **学習リソース** | 膨大 | 多い | React 勝利 |
| **求人市場** | 最大 | 中規模 | React 勝利 |
| **TypeScript対応** | 完璧 | 良好 | React 勝利 |
| **学習曲線** | やや急 | 緩やか | Vue 勝利 |
| **パフォーマンス** | 優秀 | 優秀 | 引き分け |

**結論**: 長期的なメンテナンス性、エコシステムの豊富さからReactを推奨

---

### 2. Vite を選ぶ理由

| 要素 | Vite | Create React App | Next.js |
|------|------|------------------|---------|
| **起動速度** | 超高速 | 遅い | 中速 |
| **ビルド速度** | 高速 | 中速 | 中速 |
| **設定** | シンプル | 固定的 | 複雑 |
| **HMR** | 超高速 | 中速 | 中速 |
| **バンドルサイズ** | 最小 | 大きい | 中程度 |
| **SSR** | オプション | なし | 標準 |

**結論**: 開発体験が圧倒的に良く、この用途には最適

---

### 3. Zustand を選ぶ理由

| 要素 | Zustand | Redux Toolkit | Jotai |
|------|---------|--------------|-------|
| **学習曲線** | 非常に緩やか | やや急 | 緩やか |
| **ボイラープレート** | 最小 | 中程度 | 最小 |
| **TypeScript** | 完璧 | 良好 | 完璧 |
| **DevTools** | あり | 充実 | 基本的 |
| **コードサイズ** | 1.2KB | 13KB | 3KB |

**結論**: シンプルで十分な機能、学習コスト最小

---

### 4. shadcn/ui を選ぶ理由

| 要素 | shadcn/ui | Material-UI | Ant Design |
|------|-----------|-------------|-----------|
| **カスタマイズ性** | 完全自由 | 制限あり | 制限あり |
| **バンドルサイズ** | 最小 | 大きい | 大きい |
| **デザイン** | モダン | Material | エンタープライズ |
| **TypeScript** | 完璧 | 良好 | 良好 |
| **学習曲線** | 緩やか | 中程度 | 中程度 |
| **依存関係** | 少ない | 多い | 多い |

**結論**: コピー&ペーストモデルで完全制御可能、軽量

---

## 📦 package.json 完全版

```json
{
  "name": "inspection-frontend",
  "version": "1.0.0",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "tsc && vite build",
    "preview": "vite preview",
    "test": "vitest",
    "test:ui": "vitest --ui",
    "lint": "eslint . --ext ts,tsx --report-unused-disable-directives --max-warnings 0",
    "format": "prettier --write \"src/**/*.{ts,tsx,css,md}\""
  },
  "dependencies": {
    "react": "^18.2.0",
    "react-dom": "^18.2.0",
    "react-router-dom": "^6.20.0",
    "@tanstack/react-query": "^5.14.0",
    "zustand": "^4.4.7",
    "axios": "^1.6.2",
    "recharts": "^2.10.3",
    "react-dropzone": "^14.2.3",
    "react-hook-form": "^7.49.2",
    "zod": "^3.22.4",
    "@hookform/resolvers": "^3.3.2",
    "date-fns": "^3.0.0",
    "clsx": "^2.0.0",
    "tailwind-merge": "^2.2.0",
    "lucide-react": "^0.294.0"
  },
  "devDependencies": {
    "@types/react": "^18.2.43",
    "@types/react-dom": "^18.2.17",
    "@types/node": "^20.10.5",
    "@typescript-eslint/eslint-plugin": "^6.14.0",
    "@typescript-eslint/parser": "^6.14.0",
    "@vitejs/plugin-react": "^4.2.1",
    "autoprefixer": "^10.4.16",
    "eslint": "^8.55.0",
    "eslint-plugin-react-hooks": "^4.6.0",
    "eslint-plugin-react-refresh": "^0.4.5",
    "postcss": "^8.4.32",
    "prettier": "^3.1.1",
    "tailwindcss": "^3.3.6",
    "typescript": "^5.2.2",
    "vite": "^5.0.8",
    "vitest": "^1.0.4",
    "@testing-library/react": "^14.1.2",
    "@testing-library/jest-dom": "^6.1.5",
    "@testing-library/user-event": "^14.5.1"
  }
}
```

---

## 🏗️ プロジェクト構成（完全版）

```
frontend/
├── public/
│   ├── favicon.ico
│   └── images/
├── src/
│   ├── api/
│   │   ├── client.ts              # Axiosクライアント
│   │   ├── inspection.ts          # 検査API
│   │   ├── statistics.ts          # 統計API
│   │   └── upload.ts              # アップロードAPI
│   ├── components/
│   │   ├── ui/                    # shadcn/ui components
│   │   │   ├── button.tsx
│   │   │   ├── card.tsx
│   │   │   ├── dialog.tsx
│   │   │   └── ...
│   │   ├── layout/
│   │   │   ├── Header.tsx
│   │   │   ├── Sidebar.tsx
│   │   │   ├── Footer.tsx
│   │   │   └── Layout.tsx
│   │   ├── inspection/
│   │   │   ├── ImageUpload.tsx
│   │   │   ├── ImageViewer.tsx
│   │   │   ├── DefectList.tsx
│   │   │   └── InspectionForm.tsx
│   │   ├── dashboard/
│   │   │   ├── KPICard.tsx
│   │   │   ├── ChartPanel.tsx
│   │   │   └── RecentInspections.tsx
│   │   └── settings/
│   │       ├── DetectorSettings.tsx
│   │       └── SystemSettings.tsx
│   ├── hooks/
│   │   ├── useInspection.ts       # 検査フック
│   │   ├── useStatistics.ts       # 統計フック
│   │   ├── useUpload.ts           # アップロードフック
│   │   └── useDetectors.ts        # 検出器フック
│   ├── pages/
│   │   ├── DashboardPage.tsx
│   │   ├── InspectPage.tsx
│   │   ├── ResultPage.tsx
│   │   ├── HistoryPage.tsx
│   │   ├── SettingsPage.tsx
│   │   └── NotFoundPage.tsx
│   ├── store/
│   │   ├── useAuthStore.ts        # 認証状態
│   │   ├── useSettingsStore.ts    # 設定状態
│   │   └── useUIStore.ts          # UI状態
│   ├── types/
│   │   ├── inspection.ts          # 検査関連型
│   │   ├── detector.ts            # 検出器関連型
│   │   └── api.ts                 # API型定義
│   ├── utils/
│   │   ├── cn.ts                  # classname utility
│   │   ├── format.ts              # フォーマット関数
│   │   └── constants.ts           # 定数
│   ├── App.tsx
│   ├── main.tsx
│   ├── index.css
│   └── vite-env.d.ts
├── .env.development
├── .env.production
├── .eslintrc.cjs
├── .prettierrc
├── .gitignore
├── index.html
├── package.json
├── postcss.config.js
├── tailwind.config.js
├── tsconfig.json
├── tsconfig.node.json
└── vite.config.ts
```

---

## 🚀 セットアップ手順（完全版）

### Step 1: プロジェクト作成

```bash
# Vite + React + TypeScript プロジェクト作成
npm create vite@latest frontend -- --template react-ts

cd frontend
```

### Step 2: 依存関係インストール

```bash
# コア依存関係
npm install react-router-dom @tanstack/react-query zustand axios

# UI関連
npm install clsx tailwind-merge lucide-react
npm install recharts react-dropzone

# フォーム
npm install react-hook-form zod @hookform/resolvers

# ユーティリティ
npm install date-fns

# 開発依存関係
npm install -D tailwindcss postcss autoprefixer
npm install -D @types/node
npm install -D prettier eslint-config-prettier
npm install -D vitest @testing-library/react @testing-library/jest-dom
```

### Step 3: Tailwind CSS セットアップ

```bash
npx tailwindcss init -p
```

```javascript
// tailwind.config.js
/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {},
  },
  plugins: [],
}
```

```css
/* src/index.css */
@tailwind base;
@tailwind components;
@tailwind utilities;
```

### Step 4: shadcn/ui セットアップ

```bash
# shadcn/ui CLI
npx shadcn-ui@latest init

# 必要なコンポーネントを追加
npx shadcn-ui@latest add button
npx shadcn-ui@latest add card
npx shadcn-ui@latest add dialog
npx shadcn-ui@latest add form
npx shadcn-ui@latest add input
npx shadcn-ui@latest add select
npx shadcn-ui@latest add table
npx shadcn-ui@latest add tabs
npx shadcn-ui@latest add toast
```

### Step 5: 環境変数設定

```bash
# .env.development
VITE_API_URL=http://localhost:8080
VITE_UPLOAD_MAX_SIZE=10485760
VITE_SUPPORTED_FORMATS=image/jpeg,image/png,image/bmp

# .env.production
VITE_API_URL=https://api.yourdomain.com
VITE_UPLOAD_MAX_SIZE=10485760
VITE_SUPPORTED_FORMATS=image/jpeg,image/png,image/bmp
```

### Step 6: TypeScript設定

```json
// tsconfig.json
{
  "compilerOptions": {
    "target": "ES2020",
    "useDefineForClassFields": true,
    "lib": ["ES2020", "DOM", "DOM.Iterable"],
    "module": "ESNext",
    "skipLibCheck": true,
    "moduleResolution": "bundler",
    "allowImportingTsExtensions": true,
    "resolveJsonModule": true,
    "isolatedModules": true,
    "noEmit": true,
    "jsx": "react-jsx",
    "strict": true,
    "noUnusedLocals": true,
    "noUnusedParameters": true,
    "noFallthroughCasesInSwitch": true,
    "baseUrl": ".",
    "paths": {
      "@/*": ["./src/*"]
    }
  },
  "include": ["src"],
  "references": [{ "path": "./tsconfig.node.json" }]
}
```

### Step 7: Vite設定

```typescript
// vite.config.ts
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import path from 'path'

export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
  server: {
    port: 3000,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
      },
    },
  },
})
```

### Step 8: ESLint & Prettier設定

```javascript
// .eslintrc.cjs
module.exports = {
  root: true,
  env: { browser: true, es2020: true },
  extends: [
    'eslint:recommended',
    'plugin:@typescript-eslint/recommended',
    'plugin:react-hooks/recommended',
    'prettier',
  ],
  ignorePatterns: ['dist', '.eslintrc.cjs'],
  parser: '@typescript-eslint/parser',
  plugins: ['react-refresh'],
  rules: {
    'react-refresh/only-export-components': [
      'warn',
      { allowConstantExport: true },
    ],
  },
}
```

```json
// .prettierrc
{
  "semi": false,
  "singleQuote": true,
  "trailingComma": "es5",
  "printWidth": 80,
  "tabWidth": 2
}
```

---

## 📝 実装例

### APIクライアント

```typescript
// src/api/client.ts
import axios from 'axios'

const API_BASE_URL = import.meta.env.VITE_API_URL || 'http://localhost:8080'

export const apiClient = axios.create({
  baseURL: API_BASE_URL,
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json',
  },
})

// リクエストインターセプター
apiClient.interceptors.request.use(
  (config) => {
    const token = localStorage.getItem('authToken')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => Promise.reject(error)
)

// レスポンスインターセプター
apiClient.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401) {
      localStorage.removeItem('authToken')
      window.location.href = '/login'
    }
    return Promise.reject(error)
  }
)
```

### 検査API

```typescript
// src/api/inspection.ts
import { apiClient } from './client'
import type { InspectionRequest, InspectionResult } from '@/types/inspection'

export const inspectionApi = {
  // 検査実行
  runInspection: async (request: InspectionRequest): Promise<InspectionResult> => {
    const { data } = await apiClient.post('/api/v1/inspect', request)
    return data
  },

  // 統計情報取得
  getStatistics: async () => {
    const { data } = await apiClient.get('/api/v1/statistics')
    return data
  },

  // 検出器一覧取得
  getDetectors: async () => {
    const { data } = await apiClient.get('/api/v1/detectors')
    return data
  },

  // 設定更新
  updateConfig: async (config: any) => {
    const { data } = await apiClient.post('/api/v1/config', config)
    return data
  },
}
```

### React Queryフック

```typescript
// src/hooks/useInspection.ts
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { inspectionApi } from '@/api/inspection'
import { toast } from '@/components/ui/use-toast'

export const useRunInspection = () => {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: inspectionApi.runInspection,
    onSuccess: (data) => {
      toast({
        title: '検査完了',
        description: `結果: ${data.result} (欠陥数: ${data.defect_count})`,
      })
      queryClient.invalidateQueries({ queryKey: ['statistics'] })
    },
    onError: (error) => {
      toast({
        title: '検査エラー',
        description: error.message,
        variant: 'destructive',
      })
    },
  })
}

export const useStatistics = () => {
  return useQuery({
    queryKey: ['statistics'],
    queryFn: inspectionApi.getStatistics,
    refetchInterval: 5000, // 5秒ごとに自動更新
  })
}
```

### Zustand Store

```typescript
// src/store/useSettingsStore.ts
import { create } from 'zustand'
import { persist } from 'zustand/middleware'

interface SettingsState {
  confidenceThreshold: number
  enabledDetectors: string[]
  darkMode: boolean
  setConfidenceThreshold: (value: number) => void
  toggleDetector: (detector: string) => void
  toggleDarkMode: () => void
}

export const useSettingsStore = create<SettingsState>()(
  persist(
    (set) => ({
      confidenceThreshold: 0.5,
      enabledDetectors: ['BlobDetector', 'EdgeDetector'],
      darkMode: false,
      setConfidenceThreshold: (value) =>
        set({ confidenceThreshold: value }),
      toggleDetector: (detector) =>
        set((state) => ({
          enabledDetectors: state.enabledDetectors.includes(detector)
            ? state.enabledDetectors.filter((d) => d !== detector)
            : [...state.enabledDetectors, detector],
        })),
      toggleDarkMode: () => set((state) => ({ darkMode: !state.darkMode })),
    }),
    {
      name: 'inspection-settings',
    }
  )
)
```

---

## 🎨 デザインシステム

### カラーパレット

```css
/* src/index.css */
@layer base {
  :root {
    --background: 0 0% 100%;
    --foreground: 222.2 84% 4.9%;

    --primary: 221.2 83.2% 53.3%;
    --primary-foreground: 210 40% 98%;

    --success: 142.1 76.2% 36.3%;
    --warning: 38 92% 50%;
    --error: 0 84.2% 60.2%;

    --border: 214.3 31.8% 91.4%;
    --radius: 0.5rem;
  }

  .dark {
    --background: 222.2 84% 4.9%;
    --foreground: 210 40% 98%;
    /* ... dark mode colors */
  }
}
```

---

## 💰 コスト見積（詳細版）

### 開発コスト

| 項目 | 工数 | 単価 | 小計 |
|------|------|------|------|
| フロントエンド開発 | 19日 | - | - |
| バックエンドAPI追加 | 3日 | - | - |
| テスト・QA | 2日 | - | - |
| **合計** | **24日** | - | - |

### ランニングコスト（月額）

| 項目 | 費用 |
|------|------|
| Vercel (フロントエンド) | $0（無料枠） |
| バックエンドサーバー | $44-155 |
| ドメイン | $1-2 |
| **合計** | **$45-157/月** |

---

## 📈 パフォーマンス目標

| メトリック | 目標値 |
|-----------|--------|
| **初回読み込み** | < 2秒 |
| **Time to Interactive** | < 3秒 |
| **Lighthouse Score** | > 90 |
| **バンドルサイズ** | < 500KB (gzip) |

---

## 🔒 セキュリティ対策

1. **XSS対策**: React自動エスケープ
2. **CSRF対策**: トークンベース認証
3. **API認証**: JWT + HTTPOnly Cookie
4. **HTTPS**: 必須
5. **入力検証**: Zod スキーマ検証

---

**最終決定**: React + TypeScript + Vite + shadcn/ui
**理由**: 最新技術、最高の開発体験、長期的なメンテナンス性

**作成者**: Claude Code
**最終更新**: 2025-11-21
