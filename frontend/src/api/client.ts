import axios from 'axios'

// 環境変数からAPIのベースURLを取得（開発時はNginx経由で/api/*にプロキシ）
const API_BASE_URL = import.meta.env.VITE_API_URL || '/api'

export const apiClient = axios.create({
  baseURL: API_BASE_URL,
  timeout: 30000, // 画像処理は時間がかかる可能性があるため30秒
  headers: {
    'Content-Type': 'application/json',
  },
})

// リクエストインターセプター（将来の認証用）
apiClient.interceptors.request.use(
  (config) => {
    // 将来の拡張: 認証トークンの追加
    // const token = localStorage.getItem('authToken')
    // if (token) {
    //   config.headers.Authorization = `Bearer ${token}`
    // }
    return config
  },
  (error) => Promise.reject(error)
)

// レスポンスインターセプター（エラーハンドリング）
apiClient.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401) {
      // 認証エラー時の処理（将来の拡張）
      console.error('Unauthorized access')
    } else if (error.response?.status === 500) {
      console.error('Server error:', error.response.data)
    }
    return Promise.reject(error)
  }
)
