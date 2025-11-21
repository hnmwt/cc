import { useEffect, useState } from 'react'
import { inspectionApi } from './api/inspection'
import type { ServerStatus, Statistics, InspectionResult, InspectionHistoryRecord } from './api/inspection'
import './App.css'

type TabType = 'dashboard' | 'inspect' | 'history'

function App() {
  const [activeTab, setActiveTab] = useState<TabType>('dashboard')
  const [status, setStatus] = useState<ServerStatus | null>(null)
  const [statistics, setStatistics] = useState<Statistics | null>(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  // 画像アップロード関連
  const [selectedFile, setSelectedFile] = useState<File | null>(null)
  const [previewUrl, setPreviewUrl] = useState<string | null>(null)
  const [inspectionResult, setInspectionResult] = useState<InspectionResult | null>(null)
  const [inspecting, setInspecting] = useState(false)

  // 検査履歴
  const [history, setHistory] = useState<InspectionHistoryRecord[]>([])

  useEffect(() => {
    fetchData()
    // 30秒ごとに更新
    const interval = setInterval(fetchData, 30000)
    return () => clearInterval(interval)
  }, [])

  const fetchData = async () => {
    try {
      setLoading(true)
      const [statusData, statsData, historyData] = await Promise.all([
        inspectionApi.getStatus(),
        inspectionApi.getStatistics(),
        inspectionApi.getInspectionHistory(),
      ])
      setStatus(statusData)
      setStatistics(statsData)
      setHistory(historyData)
      setError(null)
    } catch (err) {
      console.error('Failed to fetch data:', err)
      setError('Failed to connect to backend server')
    } finally {
      setLoading(false)
    }
  }

  const handleFileSelect = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (file) {
      setSelectedFile(file)
      setInspectionResult(null)

      // プレビュー生成
      const reader = new FileReader()
      reader.onloadend = () => {
        setPreviewUrl(reader.result as string)
      }
      reader.readAsDataURL(file)
    }
  }

  const handleUploadAndInspect = async () => {
    if (!selectedFile || !previewUrl) {
      alert('画像を選択してください')
      return
    }

    try {
      setInspecting(true)
      setError(null)

      // Base64データを抽出（data:image/...;base64, を除去）
      const base64Data = previewUrl.split(',')[1]

      // 1. 画像をアップロード
      const uploadResponse = await inspectionApi.uploadImage({
        image: base64Data,
        filename: selectedFile.name,
      })

      // 2. 検査を実行
      const result = await inspectionApi.runInspection({
        image_path: uploadResponse.image_path,
      })

      setInspectionResult(result)

      // 3. 履歴を更新
      await fetchData()

    } catch (err: any) {
      console.error('Inspection failed:', err)
      setError(err.response?.data?.message || '検査に失敗しました')
    } finally {
      setInspecting(false)
    }
  }

  if (loading && !status) {
    return (
      <div className="min-h-screen bg-gray-100 flex items-center justify-center">
        <div className="text-xl text-gray-600">Loading...</div>
      </div>
    )
  }

  return (
    <div className="min-h-screen bg-gray-100">
      {/* ヘッダー */}
      <header className="bg-white shadow-sm">
        <div className="max-w-7xl mx-auto px-4 py-4">
          <h1 className="text-3xl font-bold text-gray-900">
            外観検査システム
          </h1>
        </div>
      </header>

      {/* タブナビゲーション */}
      <div className="max-w-7xl mx-auto px-4 py-4">
        <div className="flex space-x-4 border-b border-gray-300">
          <button
            onClick={() => setActiveTab('dashboard')}
            className={`px-4 py-2 font-semibold ${
              activeTab === 'dashboard'
                ? 'text-blue-600 border-b-2 border-blue-600'
                : 'text-gray-600 hover:text-gray-900'
            }`}
          >
            ダッシュボード
          </button>
          <button
            onClick={() => setActiveTab('inspect')}
            className={`px-4 py-2 font-semibold ${
              activeTab === 'inspect'
                ? 'text-blue-600 border-b-2 border-blue-600'
                : 'text-gray-600 hover:text-gray-900'
            }`}
          >
            検査実行
          </button>
          <button
            onClick={() => setActiveTab('history')}
            className={`px-4 py-2 font-semibold ${
              activeTab === 'history'
                ? 'text-blue-600 border-b-2 border-blue-600'
                : 'text-gray-600 hover:text-gray-900'
            }`}
          >
            検査履歴
          </button>
        </div>
      </div>

      {/* コンテンツエリア */}
      <div className="max-w-7xl mx-auto px-4 py-6">
        {error && (
          <div className="mb-6 bg-red-100 border border-red-400 text-red-700 px-6 py-4 rounded-lg">
            <p className="font-bold">Error</p>
            <p>{error}</p>
          </div>
        )}

        {/* ダッシュボード */}
        {activeTab === 'dashboard' && (
          <div className="space-y-6">
            {/* サーバーステータス */}
            <div className="bg-white rounded-lg shadow-md p-6">
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
          </div>
        )}

        {/* 検査実行 */}
        {activeTab === 'inspect' && (
          <div className="space-y-6">
            <div className="bg-white rounded-lg shadow-md p-6">
              <h2 className="text-2xl font-semibold text-gray-800 mb-4">
                画像アップロード
              </h2>

              {/* ファイル選択 */}
              <div className="mb-6">
                <label className="block text-sm font-medium text-gray-700 mb-2">
                  検査する画像を選択
                </label>
                <input
                  type="file"
                  accept="image/*"
                  onChange={handleFileSelect}
                  className="block w-full text-sm text-gray-500 file:mr-4 file:py-2 file:px-4 file:rounded-lg file:border-0 file:text-sm file:font-semibold file:bg-blue-50 file:text-blue-700 hover:file:bg-blue-100"
                />
              </div>

              {/* プレビュー */}
              {previewUrl && (
                <div className="mb-6">
                  <p className="text-sm font-medium text-gray-700 mb-2">プレビュー</p>
                  <div className="border-2 border-gray-300 rounded-lg p-4 bg-gray-50">
                    <img
                      src={previewUrl}
                      alt="Preview"
                      className="max-w-full h-auto max-h-96 mx-auto"
                    />
                  </div>
                </div>
              )}

              {/* 検査実行ボタン */}
              <button
                onClick={handleUploadAndInspect}
                disabled={!selectedFile || inspecting}
                className={`w-full py-3 px-6 rounded-lg font-semibold text-white ${
                  !selectedFile || inspecting
                    ? 'bg-gray-400 cursor-not-allowed'
                    : 'bg-blue-600 hover:bg-blue-700'
                }`}
              >
                {inspecting ? '検査中...' : '検査を実行'}
              </button>
            </div>

            {/* 検査結果 */}
            {inspectionResult && (
              <div className="bg-white rounded-lg shadow-md p-6">
                <h2 className="text-2xl font-semibold text-gray-800 mb-4">
                  検査結果
                </h2>
                <div className="space-y-4">
                  <div className={`p-6 rounded-lg ${
                    inspectionResult.isOK ? 'bg-green-50' : 'bg-red-50'
                  }`}>
                    <p className="text-sm text-gray-600">判定</p>
                    <p className={`text-4xl font-bold ${
                      inspectionResult.isOK ? 'text-green-600' : 'text-red-600'
                    }`}>
                      {inspectionResult.isOK ? 'OK' : 'NG'}
                    </p>
                  </div>

                  <div className="grid grid-cols-2 gap-4">
                    <div className="bg-gray-50 p-4 rounded-lg">
                      <p className="text-sm text-gray-600">欠陥数</p>
                      <p className="text-2xl font-bold text-gray-800">
                        {inspectionResult.defectCount}
                      </p>
                    </div>
                    <div className="bg-gray-50 p-4 rounded-lg">
                      <p className="text-sm text-gray-600">処理時間</p>
                      <p className="text-2xl font-bold text-gray-800">
                        {inspectionResult.totalTime.toFixed(2)} ms
                      </p>
                    </div>
                  </div>

                  {inspectionResult.defects.length > 0 && (
                    <div>
                      <p className="text-sm font-medium text-gray-700 mb-2">
                        検出された欠陥:
                      </p>
                      <div className="space-y-2">
                        {inspectionResult.defects.map((defect, index) => (
                          <div key={index} className="bg-gray-50 p-3 rounded-lg">
                            <p className="text-sm">
                              <span className="font-semibold">種類:</span> {defect.type}
                              {' '}|{' '}
                              <span className="font-semibold">位置:</span> ({defect.position.x}, {defect.position.y})
                              {' '}|{' '}
                              <span className="font-semibold">サイズ:</span> {defect.size}
                            </p>
                          </div>
                        ))}
                      </div>
                    </div>
                  )}
                </div>
              </div>
            )}
          </div>
        )}

        {/* 検査履歴 */}
        {activeTab === 'history' && (
          <div className="bg-white rounded-lg shadow-md p-6">
            <div className="flex justify-between items-center mb-4">
              <h2 className="text-2xl font-semibold text-gray-800">
                検査履歴
              </h2>
              <button
                onClick={fetchData}
                className="px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700"
              >
                更新
              </button>
            </div>

            {history.length === 0 ? (
              <p className="text-gray-500 text-center py-8">
                検査履歴がありません
              </p>
            ) : (
              <div className="overflow-x-auto">
                <table className="min-w-full divide-y divide-gray-200">
                  <thead className="bg-gray-50">
                    <tr>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                        ID
                      </th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                        日時
                      </th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                        結果
                      </th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                        欠陥数
                      </th>
                      <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                        処理時間
                      </th>
                    </tr>
                  </thead>
                  <tbody className="bg-white divide-y divide-gray-200">
                    {history.map((record) => (
                      <tr key={record.id} className="hover:bg-gray-50">
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                          {record.id.slice(-8)}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                          {new Date(record.timestamp).toLocaleString('ja-JP')}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap">
                          <span className={`px-2 py-1 inline-flex text-xs leading-5 font-semibold rounded-full ${
                            record.result === 'OK'
                              ? 'bg-green-100 text-green-800'
                              : 'bg-red-100 text-red-800'
                          }`}>
                            {record.result}
                          </span>
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                          {record.defect_count}
                        </td>
                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-900">
                          {record.processing_time_ms.toFixed(2)} ms
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            )}
          </div>
        )}
      </div>

      {/* フッター */}
      <footer className="mt-8 pb-8 text-center text-gray-500 text-sm">
        <p>外観検査システム v1.0.0</p>
        <p>最終更新: {new Date().toLocaleString('ja-JP')}</p>
      </footer>
    </div>
  )
}

export default App
