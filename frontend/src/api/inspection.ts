import { apiClient } from './client'

export interface ServerStatus {
  status: string
  port: number
  controller: {
    detector_count: number
    visualization_enabled: boolean
  }
  auto_save: boolean
}

export interface Statistics {
  total_inspections: number
  ok_count: number
  ng_count: number
  defect_count: number
}

export interface InspectionRequest {
  image_path: string
}

export interface Defect {
  type: string
  position: { x: number; y: number }
  size: number
  confidence: number
}

export interface InspectionResult {
  result: 'OK' | 'NG'
  defects: Defect[]
  processing_time_ms: number
  timestamp: string
}

export const inspectionApi = {
  // サーバー稼働確認
  getRoot: async () => {
    const { data } = await apiClient.get('/v1/')
    return data
  },

  // ステータス取得
  getStatus: async (): Promise<ServerStatus> => {
    const { data } = await apiClient.get('/v1/status')
    return data
  },

  // 統計情報取得
  getStatistics: async (): Promise<Statistics> => {
    const { data } = await apiClient.get('/v1/statistics')
    return data
  },

  // 検査実行
  runInspection: async (request: InspectionRequest): Promise<InspectionResult> => {
    const { data } = await apiClient.post('/v1/inspect', request)
    return data
  },

  // 設定更新（将来の拡張）
  updateConfig: async (config: any) => {
    const { data } = await apiClient.post('/v1/config', config)
    return data
  },
}
