import { Battery, Save, Droplet, Loader2 } from 'lucide-react';
import { useState, useEffect } from 'react';
import { useAuth } from '../context/AuthContext';
import { Alert, AlertDescription } from '../components/ui/alert';

interface UserSettings {
  battery_threshold_percent: number;
  water_level_threshold_percent: number;
}

export default function Settings() {
  const { token } = useAuth();
  const [batteryThreshold, setBatteryThreshold] = useState(20);
  const [waterLevelThreshold, setWaterLevelThreshold] = useState(10);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [saved, setSaved] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    const fetchSettings = async () => {
      try {
        const res = await fetch('/api/v1/settings', {
          headers: { Authorization: `Bearer ${token}` },
        });
        if (!res.ok) throw new Error('Błąd pobierania ustawień');
        const data: UserSettings = await res.json();
        setBatteryThreshold(data.battery_threshold_percent);
        setWaterLevelThreshold(data.water_level_threshold_percent);
      } catch (e: any) {
        setError(e.message);
      } finally {
        setLoading(false);
      }
    };
    fetchSettings();
  }, [token]);

  const handleSave = async () => {
    setSaving(true);
    setError('');
    setSaved(false);
    try {
      const res = await fetch('/api/v1/settings', {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${token}`,
        },
        body: JSON.stringify({
          battery_threshold_percent: batteryThreshold,
          water_level_threshold_percent: waterLevelThreshold,
        }),
      });
      if (!res.ok) throw new Error('Błąd zapisu ustawień');
      setSaved(true);
      setTimeout(() => setSaved(false), 3000);
    } catch (e: any) {
      setError(e.message);
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="p-8">
      <div className="max-w-4xl mx-auto">
        {/* Header */}
        <div className="mb-8">
          <h1 className="text-4xl mb-2 text-slate-800">Ustawienia</h1>
          <p className="text-slate-600">Konfiguracja progów alertów urządzeń</p>
        </div>

        {loading && (
          <div className="flex items-center justify-center py-16 text-slate-500 gap-3">
            <Loader2 className="w-6 h-6 animate-spin" />
            Ładowanie ustawień…
          </div>
        )}

        {error && (
          <Alert variant="destructive" className="mb-6">
            <AlertDescription>{error}</AlertDescription>
          </Alert>
        )}

        {saved && (
          <Alert className="mb-6 border-emerald-200 bg-emerald-50 text-emerald-800">
            <AlertDescription>Ustawienia zapisane pomyślnie.</AlertDescription>
          </Alert>
        )}

        {!loading && (
          <div className="space-y-6">
            {/* Thresholds */}
            <div className="bg-white rounded-2xl shadow-lg p-6">
              <div className="flex items-center gap-3 mb-6">
                <div className="w-10 h-10 bg-gradient-to-br from-orange-500 to-orange-600 rounded-lg flex items-center justify-center">
                  <Battery className="w-5 h-5 text-white" />
                </div>
                <div>
                  <h2 className="text-xl text-slate-800">Progi ostrzeżeń</h2>
                  <p className="text-sm text-slate-500">
                    Urządzenia poniżej tych wartości pojawią się w kafelku "Wymagają uwagi"
                  </p>
                </div>
              </div>

              <div className="space-y-6">
                {/* Battery Threshold */}
                <div>
                  <div className="flex justify-between items-center mb-2">
                    <label className="text-slate-700 flex items-center gap-2">
                      <Battery className="w-4 h-4 text-green-600" />
                      Próg poziomu baterii
                    </label>
                    <span className="text-2xl text-green-600">{batteryThreshold}%</span>
                  </div>
                  <input
                    type="range" min="0" max="100" value={batteryThreshold}
                    onChange={(e) => setBatteryThreshold(Number(e.target.value))}
                    className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer"
                  />
                  <p className="text-sm text-slate-500 mt-2">
                    Urządzenie trafi do "Wymagają uwagi" gdy bateria spadnie poniżej tej wartości
                  </p>
                </div>

                {/* Water Level Threshold */}
                <div>
                  <div className="flex justify-between items-center mb-2">
                    <label className="text-slate-700 flex items-center gap-2">
                      <Droplet className="w-4 h-4 text-blue-600" />
                      Próg poziomu wody
                    </label>
                    <span className="text-2xl text-blue-600">{waterLevelThreshold}%</span>
                  </div>
                  <input
                    type="range" min="0" max="100" value={waterLevelThreshold}
                    onChange={(e) => setWaterLevelThreshold(Number(e.target.value))}
                    className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer"
                  />
                  <p className="text-sm text-slate-500 mt-2">
                    Urządzenie trafi do "Wymagają uwagi" gdy poziom wody spadnie poniżej tej wartości
                  </p>
                </div>
              </div>
            </div>

            {/* Save Button */}
            <button
              onClick={handleSave}
              disabled={saving}
              className="w-full flex items-center justify-center gap-3 px-6 py-4 bg-gradient-to-r from-emerald-500 to-teal-600 text-white rounded-xl hover:shadow-lg transition-shadow disabled:opacity-50"
            >
              {saving
                ? <><Loader2 className="w-5 h-5 animate-spin" /> Zapisywanie…</>
                : <><Save className="w-5 h-5" /> Zapisz ustawienia</>
              }
            </button>
          </div>
        )}
      </div>
    </div>
  );
}
