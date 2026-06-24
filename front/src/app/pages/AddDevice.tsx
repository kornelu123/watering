import { useState } from 'react';
import { useNavigate } from 'react-router';
import { ArrowLeft, Plus, Loader2 } from 'lucide-react';
import { useAuth } from '../context/AuthContext';
import { Alert, AlertDescription } from '../components/ui/alert';

export default function AddDevice() {
  const { token } = useAuth();
  const navigate = useNavigate();

  const [deviceId, setDeviceId] = useState('');
  const [name, setName] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);

    try {
      const res = await fetch('/api/v1/devices', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${token}`,
        },
        body: JSON.stringify({ device_id: deviceId.trim(), name: name.trim() }),
      });

      const data = await res.json();
      if (!res.ok) throw new Error(data.detail || 'Błąd dodawania urządzenia');

      navigate('/');
    } catch (err: any) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="p-8">
      <div className="max-w-xl mx-auto">
        <div className="mb-8">
          <button
            onClick={() => navigate('/')}
            className="flex items-center gap-2 text-slate-600 hover:text-slate-800 mb-4"
          >
            <ArrowLeft className="w-5 h-5" />
            Powrót do listy urządzeń
          </button>
          <h1 className="text-4xl mb-2 text-slate-800">Dodaj urządzenie</h1>
          <p className="text-slate-600">Zarejestruj nowy czujnik w systemie</p>
        </div>

        <div className="bg-white rounded-2xl shadow-lg p-8">
          <form className="space-y-6" onSubmit={handleSubmit}>
            {error && (
              <Alert variant="destructive">
                <AlertDescription>{error}</AlertDescription>
              </Alert>
            )}

            <div>
              <label className="block text-sm font-medium text-slate-700 mb-2">
                ID urządzenia
              </label>
              <input
                type="text"
                required
                value={deviceId}
                onChange={(e) => setDeviceId(e.target.value)}
                placeholder="np. PLANT-001"
                className="w-full px-4 py-3 border border-slate-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-emerald-500 font-mono"
              />
              <p className="mt-1 text-xs text-slate-500">
                To ID musi być identyczne z tym zaprogramowanym w urządzeniu fizycznym.
              </p>
            </div>

            <div>
              <label className="block text-sm font-medium text-slate-700 mb-2">
                Nazwa wyświetlana
              </label>
              <input
                type="text"
                required
                value={name}
                onChange={(e) => setName(e.target.value)}
                placeholder="np. Roślina – Salon"
                className="w-full px-4 py-3 border border-slate-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-emerald-500"
              />
            </div>

            <div className="pt-2">
              <button
                type="submit"
                disabled={loading || !deviceId.trim() || !name.trim()}
                className="w-full flex items-center justify-center gap-3 px-6 py-4 bg-gradient-to-r from-emerald-500 to-teal-600 text-white rounded-xl hover:shadow-lg transition-shadow disabled:opacity-50 disabled:cursor-not-allowed"
              >
                {loading
                  ? <><Loader2 className="w-5 h-5 animate-spin" /> Dodawanie…</>
                  : <><Plus className="w-5 h-5" /> Zarejestruj urządzenie</>
                }
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
}
