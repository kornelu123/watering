import { useParams, useNavigate } from 'react-router';
import { ArrowLeft, Save, Droplet, Clock, RotateCw, Loader2 } from 'lucide-react';
import { useState, useEffect } from 'react';
import { useAuth } from '../context/AuthContext';
import { Alert, AlertDescription } from '../components/ui/alert';
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
} from '../components/ui/alert-dialog';

interface DeviceDetail {
  device_id: string;
  name: string;
  water_duration_sec: number;
  moisture_threshold_percent: number;
}

export default function DeviceSettings() {
  const { deviceId } = useParams();
  const navigate = useNavigate();
  const { token } = useAuth();

  const [device, setDevice] = useState<DeviceDetail | null>(null);
  const [name, setName] = useState('');
  const [moistureThreshold, setMoistureThreshold] = useState(30);
  const [wateringDuration, setWateringDuration] = useState(10);
  const [loadingDevice, setLoadingDevice] = useState(true);
  const [saving, setSaving] = useState(false);
  const [rebooting, setRebooting] = useState(false);
  const [rebootConfirmOpen, setRebootConfirmOpen] = useState(false);
  const [error, setError] = useState('');
  const [saved, setSaved] = useState(false);
  const [commandMessage, setCommandMessage] = useState('');

  useEffect(() => {
    if (!deviceId) return;
    const fetchDevice = async () => {
      try {
        const res = await fetch(`/api/v1/devices/${deviceId}`, {
          headers: { Authorization: `Bearer ${token}` },
        });
        if (!res.ok) throw new Error('Nie można pobrać urządzenia');
        const data: DeviceDetail = await res.json();
        setDevice(data);
        setName(data.name);
        setMoistureThreshold(data.moisture_threshold_percent);
        setWateringDuration(data.water_duration_sec);
      } catch (e: any) {
        setError(e.message);
      } finally {
        setLoadingDevice(false);
      }
    };
    fetchDevice();
  }, [deviceId, token]);

  const handleSave = async () => {
    setSaving(true);
    setError('');
    setSaved(false);
    try {
      const res = await fetch(`/api/v1/devices/${deviceId}/settings`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${token}`,
        },
        body: JSON.stringify({
          name,
          water_duration_sec: wateringDuration,
          moisture_threshold_percent: moistureThreshold,
        }),
      });
      const data = await res.json();
      if (!res.ok) throw new Error(data.detail || 'Błąd zapisu');
      setSaved(true);
      setTimeout(() => setSaved(false), 3000);
    } catch (e: any) {
      setError(e.message);
    } finally {
      setSaving(false);
    }
  };

  const handleRestart = async () => {
    setRebootConfirmOpen(false);
    setRebooting(true);
    setError('');
    setCommandMessage('');
    try {
      const res = await fetch(`/api/v1/devices/${deviceId}/reboot`, {
        method: 'POST',
        headers: { Authorization: `Bearer ${token}` },
      });
      if (!res.ok) throw new Error('Błąd restartu');
      setCommandMessage('Komenda restartu wysłana!');
    } catch (e: any) {
      setError(e.message);
    } finally {
      setRebooting(false);
    }
  };

  return (
    <div className="p-8">
      <div className="max-w-4xl mx-auto">
        {/* Header */}
        <div className="mb-8">
          <button
            onClick={() => navigate(`/devices/${deviceId}`)}
            className="flex items-center gap-2 text-slate-600 hover:text-slate-800 mb-4"
          >
            <ArrowLeft className="w-5 h-5" />
            Powrót do analityki
          </button>
          <h1 className="text-4xl mb-2 text-slate-800">Ustawienia Urządzenia</h1>
          <p className="text-slate-600">{deviceId}</p>
        </div>

        {loadingDevice && (
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
            <AlertDescription>
              Ustawienia zapisane i komendy wysłane do urządzenia.
            </AlertDescription>
          </Alert>
        )}

        {commandMessage && (
          <Alert className="mb-6 border-emerald-200 bg-emerald-50 text-emerald-800">
            <AlertDescription>{commandMessage}</AlertDescription>
          </Alert>
        )}

        {!loadingDevice && device && (
          <div className="space-y-6">
            {/* Basic Settings */}
            <div className="bg-white rounded-2xl shadow-lg p-6">
              <h2 className="text-xl text-slate-800 mb-6">Podstawowe informacje</h2>
              <div>
                <label className="block text-slate-700 mb-2">Nazwa urządzenia</label>
                <input
                  type="text"
                  value={name}
                  onChange={(e) => setName(e.target.value)}
                  className="w-full px-4 py-3 border border-slate-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-emerald-500"
                  placeholder="Wprowadź nazwę urządzenia"
                />
              </div>
            </div>

            {/* Watering Settings */}
            <div className="bg-white rounded-2xl shadow-lg p-6">
              <div className="flex items-center gap-3 mb-6">
                <div className="w-10 h-10 bg-gradient-to-br from-blue-500 to-blue-600 rounded-lg flex items-center justify-center">
                  <Droplet className="w-5 h-5 text-white" />
                </div>
                <div>
                  <h2 className="text-xl text-slate-800">Ustawienia podlewania</h2>
                  <p className="text-sm text-slate-500">Zmiana wysyła komendę do urządzenia</p>
                </div>
              </div>

              <div className="space-y-6">
                {/* Moisture Threshold */}
                <div>
                  <div className="flex justify-between items-center mb-2">
                    <label className="text-slate-700">Próg wilgotności gleby</label>
                    <span className="text-2xl text-blue-600">{moistureThreshold}%</span>
                  </div>
                  <input
                    type="range" min="0" max="100" value={moistureThreshold}
                    onChange={(e) => setMoistureThreshold(Number(e.target.value))}
                    className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer"
                  />
                  <p className="text-sm text-slate-500 mt-2">
                    Podlewanie zostanie uruchomione gdy wilgotność spadnie poniżej tej wartości
                  </p>
                </div>

                {/* Watering Duration */}
                <div>
                  <div className="flex justify-between items-center mb-2">
                    <label className="text-slate-700">Czas podlewania</label>
                    <span className="text-2xl text-blue-600">{wateringDuration}s</span>
                  </div>
                  <input
                    type="range" min="1" max="60" value={wateringDuration}
                    onChange={(e) => setWateringDuration(Number(e.target.value))}
                    className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer"
                  />
                  <p className="text-sm text-slate-500 mt-2">
                    Długość pojedynczego cyklu podlewania w sekundach
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

            {/* Reboot Button */}
            <button
              onClick={() => setRebootConfirmOpen(true)}
              disabled={rebooting}
              className="w-full flex items-center justify-center gap-3 px-6 py-4 bg-white border border-orange-200 text-orange-600 rounded-xl hover:bg-orange-50 transition-colors disabled:opacity-50"
            >
              {rebooting
                ? <><Loader2 className="w-5 h-5 animate-spin" /> Wysyłanie…</>
                : <><RotateCw className="w-5 h-5" /> Uruchom ponownie</>
              }
            </button>
          </div>
        )}
      </div>

        <AlertDialog open={rebootConfirmOpen} onOpenChange={setRebootConfirmOpen}>
          <AlertDialogContent className="sm:max-w-md">
            <AlertDialogHeader>
              <AlertDialogTitle>Uruchomić urządzenie ponownie?</AlertDialogTitle>
              <AlertDialogDescription>
                Ta akcja wyśle komendę restartu do urządzenia {deviceId}. Nie używamy natywnego popupu, tylko spójny dialog aplikacji.
              </AlertDialogDescription>
            </AlertDialogHeader>
            <AlertDialogFooter>
              <AlertDialogCancel>Anuluj</AlertDialogCancel>
              <AlertDialogAction onClick={handleRestart}>Uruchom ponownie</AlertDialogAction>
            </AlertDialogFooter>
          </AlertDialogContent>
        </AlertDialog>
    </div>
  );
}
