import { useEffect, useState } from 'react'
import { inspectionApi } from './api/inspection'
import type { ServerStatus, Statistics } from './api/inspection'
import './App.css'

function App() {
  const [status, setStatus] = useState<ServerStatus | null>(null)
  const [statistics, setStatistics] = useState<Statistics | null>(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    const fetchData = async () => {
      try {
        setLoading(true)
        const [statusData, statsData] = await Promise.all([
          inspectionApi.getStatus(),
          inspectionApi.getStatistics(),
        ])
        setStatus(statusData)
        setStatistics(statsData)
        setError(null)
      } catch (err) {
        console.error('Failed to fetch data:', err)
        setError('Failed to connect to backend server')
      } finally {
        setLoading(false)
      }
    }

    fetchData()
    // 10秒ごとに更新
    const interval = setInterval(fetchData, 10000)
    return () => clearInterval(interval)
  }, [])

  if (loading) {
    return (
      <div className="min-h-screen bg-gray-100 flex items-center justify-center">
        <div className="text-xl text-gray-600">Loading...</div>
      </div>
    )
  }

  if (error) {
    return (
      <div className="min-h-screen bg-gray-100 flex items-center justify-center">
        <div className="bg-red-100 border border-red-400 text-red-700 px-6 py-4 rounded-lg">
          <p className="font-bold">Error</p>
          <p>{error}</p>
        </div>
      </div>
    )
  }

  return (
    <div className="min-h-screen bg-gray-100 py-8 px-4">
      <div className="max-w-7xl mx-auto">
        <h1 className="text-4xl font-bold text-gray-900 mb-8">
          外観検査システム ダッシュボード
        </h1>

        {/* サーバーステータス */}
        <div className="bg-white rounded-lg shadow-md p-6 mb-6">
          <h2 className="text-2xl font-semibold text-gray-800 mb-4">
            サーバーステータス
          </h2>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
            <div className="bg-green-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">ステータス</p>
              <p className="text-xl font-bold text-green-600">
                {status?.status?.toUpperCase()}
              </p>
            </div>
            <div className="bg-blue-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">ポート</p>
              <p className="text-xl font-bold text-blue-600">{status?.port}</p>
            </div>
            <div className="bg-purple-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">検出器数</p>
              <p className="text-xl font-bold text-purple-600">
                {status?.controller.detector_count}
              </p>
            </div>
            <div className="bg-yellow-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">自動保存</p>
              <p className="text-xl font-bold text-yellow-600">
                {status?.auto_save ? 'ON' : 'OFF'}
              </p>
            </div>
          </div>
        </div>

        {/* 検査統計 */}
        <div className="bg-white rounded-lg shadow-md p-6">
          <h2 className="text-2xl font-semibold text-gray-800 mb-4">
            検査統計
          </h2>
          <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
            <div className="bg-gray-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">総検査数</p>
              <p className="text-2xl font-bold text-gray-800">
                {statistics?.total_inspections ?? 0}
              </p>
            </div>
            <div className="bg-green-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">OK数</p>
              <p className="text-2xl font-bold text-green-600">
                {statistics?.ok_count ?? 0}
              </p>
            </div>
            <div className="bg-red-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">NG数</p>
              <p className="text-2xl font-bold text-red-600">
                {statistics?.ng_count ?? 0}
              </p>
            </div>
            <div className="bg-orange-50 p-4 rounded-lg">
              <p className="text-sm text-gray-600">総欠陥数</p>
              <p className="text-2xl font-bold text-orange-600">
                {statistics?.defect_count ?? 0}
              </p>
            </div>
          </div>

          {/* 合格率 */}
          {statistics && statistics.total_inspections > 0 && (
            <div className="mt-6 p-4 bg-blue-50 rounded-lg">
              <p className="text-sm text-gray-600 mb-2">合格率</p>
              <div className="w-full bg-gray-200 rounded-full h-6">
                <div
                  className="bg-green-500 h-6 rounded-full flex items-center justify-center text-white text-sm font-semibold"
                  style={{
                    width: `${
                      (statistics.ok_count / statistics.total_inspections) * 100
                    }%`,
                  }}
                >
                  {(
                    (statistics.ok_count / statistics.total_inspections) *
                    100
                  ).toFixed(1)}
                  %
                </div>
              </div>
            </div>
          )}
        </div>

        {/* フッター */}
        <div className="mt-8 text-center text-gray-500 text-sm">
          <p>外観検査システム v1.0.0</p>
          <p>自動更新: 10秒ごと</p>
        </div>
      </div>
    </div>
  )
}

export default App
