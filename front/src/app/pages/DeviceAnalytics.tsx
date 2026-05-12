import { useParams, useNavigate } from 'react-router';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { TrendingUp, Calendar, ArrowLeft, Settings as SettingsIcon, Loader2 } from 'lucide-react';
import { useState, useEffect } from 'react';
import { useAuth } from '../context/AuthContext';

interface TelemetryPoint {
  time: string;
  moisture_lvl: number;
  battery_lvl: number;
  water_lvl: number;
}

export default function DeviceAnalytics() {
  const { deviceId } = useParams();
  const navigate = useNavigate();
  const { token } = useAuth();

  const [timeRange, setTimeRange] = useState('24h');
  const [data, setData] = useState<TelemetryPoint[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  useEffect(() => {
    if (!deviceId) return;
    const fetchHistory = async () => {
      setLoading(true);
      setError('');
      try {
        const res = await fetch(`/api/v1/telemetry/${deviceId}?range=${timeRange}`, {
          headers: { Authorization: `Bearer ${token}` },
        });
        if (!res.ok) throw new Error('Błąd pobierania danych historycznych');
        setData(await res.json());
      } catch (e: any) {
        setError(e.message);
      } finally {
        setLoading(false);
      }
    };
    fetchHistory();
  }, [deviceId, timeRange, token]);

  const rangeLabel = timeRange === '24h' ? 'Ostatnie 24 godziny' : timeRange === '7d' ? 'Ostatnie 7 dni' : 'Ostatnie 30 dni';

  const formatTime = (iso: string) => {
    const d = new Date(iso);
    const dd = String(d.getDate()).padStart(2, '0');
    const mm = String(d.getMonth() + 1).padStart(2, '0');
    const hh = String(d.getHours()).padStart(2, '0');
    const min = String(d.getMinutes()).padStart(2, '0');
    if (timeRange === '24h') return `${hh}:${min}`;
    if (timeRange === '7d') return `${dd}.${mm} ${hh}:${min}`;
    return `${dd}.${mm}`;
  };

  // keep original ISO in data and format labels on the chart (so tick/tooltip can show time+date)
  const chartData = data.map(p => ({
    time: p.time,
    wilgotnosc: p.moisture_lvl,
    bateria: p.battery_lvl,
    woda: p.water_lvl,
  }));

  return (
    <div className="p-8">
      <div className="max-w-6xl mx-auto">
        {/* Header */}
        <div className="mb-8 flex justify-between items-start">
          <div>
            <button
              onClick={() => navigate('/')}
              className="flex items-center gap-2 text-slate-600 hover:text-slate-800 mb-4"
            >
              <ArrowLeft className="w-5 h-5" />
              Powrót do listy urządzeń
            </button>
            <h1 className="text-4xl mb-2 text-slate-800">Analityka Urządzenia</h1>
            <p className="text-slate-600">{deviceId}</p>
          </div>
          <button
            onClick={() => navigate(`/devices/${deviceId}/settings`)}
            className="flex items-center gap-2 px-4 py-2 bg-white border border-slate-300 text-slate-700 rounded-lg hover:shadow transition-shadow"
          >
            <SettingsIcon className="w-5 h-5" />
            Ustawienia
          </button>
        </div>

        {/* Date Range Selector */}
        <div className="bg-white rounded-2xl shadow-lg p-6 mb-6">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-3">
              <Calendar className="w-5 h-5 text-slate-600" />
              <span className="text-slate-700">Zakres danych:</span>
            </div>
            <div className="flex gap-2">
              {(['24h', '7d', '30d'] as const).map(r => (
                <button
                  key={r}
                  onClick={() => setTimeRange(r)}
                  className={`px-4 py-2 rounded-lg ${timeRange === r ? 'bg-emerald-100 text-emerald-700' : 'text-slate-600 hover:bg-slate-100'}`}
                >
                  {r === '24h' ? '24h' : r === '7d' ? '7 dni' : '30 dni'}
                </button>
              ))}
            </div>
          </div>
        </div>

        {/* Loading / Error */}
        {loading && (
          <div className="flex items-center justify-center py-16 text-slate-500 gap-3">
            <Loader2 className="w-6 h-6 animate-spin" />
            Ładowanie danych…
          </div>
        )}

        {error && (
          <div className="bg-red-50 border border-red-200 text-red-600 px-4 py-3 rounded-lg mb-4">
            {error}
          </div>
        )}

        {!loading && !error && chartData.length === 0 && (
          <div className="text-center py-16 text-slate-400">
            Brak danych telemetrycznych dla wybranego zakresu.
          </div>
        )}

        {/* Charts */}
        {!loading && chartData.length > 0 && (
          <div className="space-y-6">
            {/* Soil Moisture Chart */}
            <div className="bg-white rounded-2xl shadow-lg p-6">
              <div className="flex items-center gap-3 mb-6">
                <div className="w-10 h-10 bg-gradient-to-br from-blue-500 to-blue-600 rounded-lg flex items-center justify-center">
                  <TrendingUp className="w-5 h-5 text-white" />
                </div>
                <div>
                  <h2 className="text-xl text-slate-800">Wilgotność Gleby</h2>
                  <p className="text-sm text-slate-500">{rangeLabel}</p>
                </div>
              </div>
              <ResponsiveContainer width="100%" height={300}>
                <LineChart data={chartData}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#e2e8f0" />
                  <XAxis dataKey="time" stroke="#64748b" style={{ fontSize: '12px' }} tickFormatter={formatTime} />
                  <YAxis stroke="#64748b" style={{ fontSize: '12px' }} domain={[0, 100]} />
                  <Tooltip labelFormatter={formatTime} contentStyle={{ backgroundColor: '#fff', border: '1px solid #e2e8f0', borderRadius: '8px' }} />
                  <Legend />
                  <Line type="monotone" dataKey="wilgotnosc" stroke="#3b82f6" strokeWidth={2} dot={false} name="Wilgotność (%)" />
                </LineChart>
              </ResponsiveContainer>
            </div>

            {/* Battery Level Chart */}
            <div className="bg-white rounded-2xl shadow-lg p-6">
              <div className="flex items-center gap-3 mb-6">
                <div className="w-10 h-10 bg-gradient-to-br from-green-500 to-green-600 rounded-lg flex items-center justify-center">
                  <TrendingUp className="w-5 h-5 text-white" />
                </div>
                <div>
                  <h2 className="text-xl text-slate-800">Poziom Baterii</h2>
                  <p className="text-sm text-slate-500">{rangeLabel}</p>
                </div>
              </div>
              <ResponsiveContainer width="100%" height={300}>
                <LineChart data={chartData}>
                  <CartesianGrid strokeDasharray="3 3" stroke="#e2e8f0" />
                  <XAxis dataKey="time" stroke="#64748b" style={{ fontSize: '12px' }} tickFormatter={formatTime} />
                  <YAxis stroke="#64748b" style={{ fontSize: '12px' }} domain={[0, 100]} />
                  <Tooltip labelFormatter={formatTime} contentStyle={{ backgroundColor: '#fff', border: '1px solid #e2e8f0', borderRadius: '8px' }} />
                  <Legend />
                  <Line type="monotone" dataKey="bateria" stroke="#22c55e" strokeWidth={2} dot={false} name="Bateria (%)" />
                </LineChart>
              </ResponsiveContainer>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
