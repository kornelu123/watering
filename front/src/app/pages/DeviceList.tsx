import { Droplet, Battery, Signal, Plus, FolderPlus, ChevronRight, Settings as SettingsIcon, Waves, Play, Loader2, Trash2 } from 'lucide-react';
import { useNavigate } from 'react-router';
import { useState, useEffect } from 'react';
import { useAuth } from '../context/AuthContext';
import { Dialog, DialogContent, DialogHeader, DialogTitle } from '../components/ui/dialog';
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

interface Device {
  device_id: string;
  name: string;
  water_duration_sec: number;
  created_at: string;
  group_id: number | null;
  group_name: string | null;
  moisture_lvl: number | null;
  battery_lvl: number | null;
  water_lvl: number | null;
  last_seen: string | null;
}

interface DeviceAssignment {
  device_id: string;
  device_name: string;
}

interface DeviceGroup {
  id: number;
  name: string;
  moisture_threshold_percent: number;
  watering_duration_sec: number;
  created_at: string;
  devices: DeviceAssignment[];
}

interface UserSettings {
  battery_threshold_percent: number;
  water_level_threshold_percent: number;
}

export default function DeviceList() {
  const [groups, setGroups] = useState<DeviceGroup[]>([]);
  const [devices, setDevices] = useState<Device[]>([]);
  const [userSettings, setUserSettings] = useState<UserSettings>({ battery_threshold_percent: 20, water_level_threshold_percent: 10 });
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [notice, setNotice] = useState('');
  const [savingGroup, setSavingGroup] = useState(false);
  const [groupDialogOpen, setGroupDialogOpen] = useState(false);
  const [activeGroup, setActiveGroup] = useState<DeviceGroup | null>(null);
  const [groupName, setGroupName] = useState('');
  const [groupMoistureThreshold, setGroupMoistureThreshold] = useState(30);
  const [groupWateringDuration, setGroupWateringDuration] = useState(10);
  const [groupSelectedDeviceIds, setGroupSelectedDeviceIds] = useState<Set<string>>(new Set());
  const [waterConfirmDeviceId, setWaterConfirmDeviceId] = useState<string | null>(null);
  const [deleteConfirmOpen, setDeleteConfirmOpen] = useState(false);

  const { token } = useAuth();
  const navigate = useNavigate();

  useEffect(() => {
    const fetchData = async () => {
      try {
        setLoading(true);
        const headers = { Authorization: `Bearer ${token}` };
        const [devicesRes, groupsRes, settingsRes] = await Promise.all([
          fetch('/api/v1/devices', { headers }),
          fetch('/api/v1/groups', { headers }),
          fetch('/api/v1/settings', { headers }),
        ]);
        if (!devicesRes.ok) throw new Error('Błąd pobierania urządzeń');
        if (!groupsRes.ok) throw new Error('Błąd pobierania grup');
        setDevices(await devicesRes.json());
        setGroups(await groupsRes.json());
        if (settingsRes.ok) setUserSettings(await settingsRes.json());
      } catch (e: any) {
        setError(e.message);
      } finally {
        setLoading(false);
      }
    };
    fetchData();
  }, [token]);

  const apiRequest = async <T,>(url: string, options: RequestInit = {}): Promise<T | null> => {
    const headers = {
      Authorization: `Bearer ${token}`,
      'Content-Type': 'application/json',
      ...(options.headers ?? {}),
    } as HeadersInit;
    const res = await fetch(url, { ...options, headers });
    if (!res.ok) {
      const payload = await res.json().catch(() => null);
      throw new Error(payload?.detail ?? 'Błąd żądania');
    }
    if (res.status === 204) return null;
    return res.json();
  };

  const reloadData = async () => {
    setError('');
    setNotice('');
    const [devicesData, groupsData] = await Promise.all([
      apiRequest<Device[]>('/api/v1/devices'),
      apiRequest<DeviceGroup[]>('/api/v1/groups'),
    ]);
    setDevices(devicesData ?? []);
    setGroups(groupsData ?? []);
  };

  const resetGroupForm = (group: DeviceGroup | null) => {
    setActiveGroup(group);
    setGroupName(group?.name ?? '');
    setGroupMoistureThreshold(group?.moisture_threshold_percent ?? 30);
    setGroupWateringDuration(group?.watering_duration_sec ?? 10);
    setGroupSelectedDeviceIds(new Set(group?.devices.map(d => d.device_id) ?? []));
  };

  const openCreateGroupDialog = () => {
    resetGroupForm(null);
    setGroupDialogOpen(true);
  };

  const openEditGroupDialog = (group: DeviceGroup) => {
    resetGroupForm(group);
    setGroupDialogOpen(true);
  };

  const closeGroupDialog = () => {
    setGroupDialogOpen(false);
    setActiveGroup(null);
    setGroupName('');
    setGroupMoistureThreshold(30);
    setGroupWateringDuration(10);
    setGroupSelectedDeviceIds(new Set());
  };

  const getStatusColor = (last_seen: string | null) => {
    if (!last_seen) return 'bg-slate-100 text-slate-500';
    const diffMin = (Date.now() - new Date(last_seen).getTime()) / 60000;
    return diffMin < 15
      ? 'bg-emerald-100 text-emerald-700'
      : 'bg-red-100 text-red-700';
  };

  const getStatusLabel = (last_seen: string | null) => {
    if (!last_seen) return 'Brak danych';
    const diffMin = (Date.now() - new Date(last_seen).getTime()) / 60000;
    return diffMin < 15 ? 'Online' : 'Offline';
  };

  const getMoistureColor = (level: number | null) => {
    if (level === null) return 'text-slate-400';
    if (level >= 60) return 'text-blue-600';
    if (level >= 30) return 'text-orange-500';
    return 'text-red-600';
  };

  const getBatteryColor = (level: number | null) => {
    if (level === null) return 'text-slate-400';
    if (level >= 60) return 'text-green-600';
    if (level >= 30) return 'text-orange-500';
    return 'text-red-600';
  };

  const handleForceWatering = async (e: React.MouseEvent, deviceId: string) => {
    e.stopPropagation();
    setWaterConfirmDeviceId(deviceId);
  };

  const confirmForceWatering = async () => {
    if (!waterConfirmDeviceId) return;
    const deviceId = waterConfirmDeviceId;
    setWaterConfirmDeviceId(null);
    try {
      await apiRequest(`/api/v1/devices/${deviceId}/water`, { method: 'POST' });
      setError('');
      setNotice('Komenda podlewania wysłana!');
    } catch (e: any) {
      setNotice('');
      setError(e.message);
    }
  };

  const handleSaveGroup = async () => {
    if (!groupName.trim()) {
      setError('Nazwa grupy nie może być pusta.');
      return;
    }

    setSavingGroup(true);
    setError('');

    try {
      if (activeGroup) {
        // Update group settings
        await apiRequest<DeviceGroup>(`/api/v1/groups/${activeGroup.id}`, {
          method: 'PUT',
          body: JSON.stringify({
            name: groupName,
            moisture_threshold_percent: groupMoistureThreshold,
            watering_duration_sec: groupWateringDuration,
          }),
        });

        // Handle device assignments
        const previousDeviceIds = new Set(activeGroup.devices.map(d => d.device_id));
        
        // Remove devices that are no longer selected
        for (const deviceId of previousDeviceIds) {
          if (!groupSelectedDeviceIds.has(deviceId)) {
            await apiRequest(`/api/v1/groups/${activeGroup.id}/devices/${deviceId}`, { method: 'DELETE' });
          }
        }
        
        // Add devices that are newly selected
        for (const deviceId of groupSelectedDeviceIds) {
          if (!previousDeviceIds.has(deviceId)) {
            await apiRequest(`/api/v1/groups/${activeGroup.id}/devices/${deviceId}`, { method: 'POST' });
          }
        }

        closeGroupDialog();
        await reloadData();
      } else {
        // Create new group
        const createdGroup = await apiRequest<DeviceGroup>('/api/v1/groups', {
          method: 'POST',
          body: JSON.stringify({
            name: groupName,
            moisture_threshold_percent: groupMoistureThreshold,
            watering_duration_sec: groupWateringDuration,
          }),
        });
        
        // Assign devices to the new group
        if (createdGroup) {
          for (const deviceId of groupSelectedDeviceIds) {
            await apiRequest(`/api/v1/groups/${createdGroup.id}/devices/${deviceId}`, { method: 'POST' });
          }
        }

        closeGroupDialog();
        await reloadData();
      }
    } catch (e: any) {
      setError(e.message);
    } finally {
      setSavingGroup(false);
    }
  };

  const handleDeleteGroup = async () => {
    if (!activeGroup) return;

    setSavingGroup(true);
    setError('');
    try {
      await apiRequest(`/api/v1/groups/${activeGroup.id}`, { method: 'DELETE' });
      closeGroupDialog();
      await reloadData();
    } catch (e: any) {
      setError(e.message);
    } finally {
      setSavingGroup(false);
    }
  };

  const openDeleteConfirm = () => {
    setDeleteConfirmOpen(true);
  };

  const onlineCount = devices.filter(d => getStatusLabel(d.last_seen) === 'Online').length;
  const attentionCount = devices.filter(d =>
    (d.battery_lvl ?? 100) < userSettings.battery_threshold_percent ||
    (d.water_lvl ?? 100) < userSettings.water_level_threshold_percent
  ).length;

  return (
    <div className="p-8">
      <div className="max-w-6xl mx-auto">
        <div className="mb-8 flex items-center justify-between">
          <div>
            <h1 className="text-4xl mb-2 text-slate-800">Urządzenia</h1>
            <p className="text-slate-600">Zarządzaj wszystkimi czujnikami</p>
          </div>
          <div className="flex gap-3">
            <button
              onClick={openCreateGroupDialog}
              className="flex items-center gap-2 px-6 py-3 bg-white border border-slate-300 text-slate-700 rounded-xl hover:shadow-lg transition-shadow"
            >
              <FolderPlus className="w-5 h-5" />
              Nowa grupa
            </button>
            <button
              onClick={() => navigate('/devices/new')}
              className="flex items-center gap-2 px-6 py-3 bg-gradient-to-r from-emerald-500 to-teal-600 text-white rounded-xl hover:shadow-lg transition-shadow"
            >
              <Plus className="w-5 h-5" />
              Dodaj urządzenie
            </button>
          </div>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-8">
          <div className="bg-white rounded-xl shadow-lg p-6">
            <div className="text-sm text-slate-600 mb-1">Wszystkie urządzenia</div>
            <div className="text-3xl text-slate-800">{devices.length}</div>
          </div>
          <div className="bg-white rounded-xl shadow-lg p-6">
            <div className="text-sm text-slate-600 mb-1">Online</div>
            <div className="text-3xl text-emerald-600">{onlineCount}</div>
          </div>
          <div className="bg-white rounded-xl shadow-lg p-6">
            <div className="text-sm text-slate-600 mb-1">Wymagają uwagi</div>
            <div className="text-3xl text-orange-600">{attentionCount}</div>
          </div>
        </div>

        {groups.length > 0 && (
          <div className="mb-8">
            <h2 className="text-2xl text-slate-800 mb-4">Grupy urządzeń</h2>
            <div className="space-y-4">
              {groups.map((group) => (
                <div key={group.id} className="bg-white rounded-xl shadow-lg p-6">
                  <div className="flex items-center justify-between gap-4 mb-4">
                    <div className="flex items-center gap-4 min-w-0">
                      <div className="w-12 h-12 bg-gradient-to-br from-purple-500 to-purple-600 rounded-xl flex items-center justify-center shrink-0">
                        <FolderPlus className="w-6 h-6 text-white" />
                      </div>
                      <div className="min-w-0">
                        <h3 className="text-lg text-slate-800 truncate">{group.name}</h3>
                        <p className="text-sm text-slate-500">
                          Próg: {group.moisture_threshold_percent}% • Czas: {group.watering_duration_sec}s
                        </p>
                        <p className="text-sm text-slate-500 mt-1 font-medium">
                          Urządzenia w grupie: {group.devices.length}
                        </p>
                      </div>
                    </div>
                    <div className="flex items-center gap-2 shrink-0">
                      <button
                        onClick={() => openEditGroupDialog(group)}
                        className="flex items-center gap-2 px-4 py-2 text-slate-600 hover:bg-slate-100 rounded-lg transition-colors"
                      >
                        <SettingsIcon className="w-5 h-5" />
                        Ustawienia
                      </button>
                    </div>
                  </div>
                  
                  {group.devices.length > 0 && (
                    <div className="mt-4 pt-4 border-t border-slate-200">
                      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
                        {group.devices.map((device) => {
                          const fullDevice = devices.find(d => d.device_id === device.device_id);
                          return (
                            <div key={device.device_id} className="bg-slate-50 rounded-lg p-3 border border-slate-200">
                              <h4 className="font-medium text-slate-800 text-sm truncate">{device.device_name}</h4>
                              <p className="text-xs text-slate-500 truncate">{device.device_id}</p>
                              {fullDevice && (
                                <div className="mt-2 flex items-center gap-2 text-xs">
                                  <span className={`px-2 py-1 rounded ${getStatusColor(fullDevice.last_seen)}`}>
                                    {getStatusLabel(fullDevice.last_seen)}
                                  </span>
                                </div>
                              )}
                            </div>
                          );
                        })}
                      </div>
                    </div>
                  )}
                </div>
              ))}
            </div>
          </div>
        )}

        <h2 className="text-2xl text-slate-800 mb-4">Wszystkie urządzenia</h2>

        {loading && (
          <div className="flex items-center justify-center py-16 text-slate-500 gap-3">
            <Loader2 className="w-6 h-6 animate-spin" />
            Ładowanie urządzeń…
          </div>
        )}

        {error && (
          <div className="bg-red-50 border border-red-200 text-red-600 px-4 py-3 rounded-lg mb-4">
            {error}
          </div>
        )}

        {notice && (
          <div className="bg-emerald-50 border border-emerald-200 text-emerald-700 px-4 py-3 rounded-lg mb-4">
            {notice}
          </div>
        )}

        {!loading && !error && devices.length === 0 && (
          <div className="text-center py-16 text-slate-400">
            Brak zarejestrowanych urządzeń.
          </div>
        )}

        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          {devices.map((device) => (
            <div
              key={device.device_id}
              onClick={() => navigate(`/devices/${device.device_id}`)}
              className="block bg-white rounded-xl shadow-lg p-6 hover:shadow-xl transition-shadow cursor-pointer"
            >
              <div className="flex items-center justify-between mb-4 gap-4">
                <div className="flex items-center gap-4 min-w-0">
                  <div className="w-12 h-12 bg-gradient-to-br from-emerald-500 to-teal-600 rounded-xl flex items-center justify-center shrink-0">
                    <Signal className="w-6 h-6 text-white" />
                  </div>
                  <div className="min-w-0">
                    <h3 className="text-lg text-slate-800 truncate">{device.name}</h3>
                    <p className="text-sm text-slate-500 truncate">{device.device_id}</p>
                    {device.group_name && (
                      <span className="mt-2 inline-flex px-2.5 py-1 rounded-full text-xs bg-emerald-100 text-emerald-700">
                        {device.group_name}
                      </span>
                    )}
                  </div>
                </div>
                <div className="flex items-center gap-3 shrink-0">
                  <span className={`px-3 py-1 rounded-full text-sm ${getStatusColor(device.last_seen)}`}>
                    {getStatusLabel(device.last_seen)}
                  </span>
                  {device.last_seen && (
                    <span className="text-sm text-slate-500">
                      {new Date(device.last_seen).toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit' })}
                    </span>
                  )}
                  <ChevronRight className="w-5 h-5 text-slate-400" />
                </div>
              </div>

              <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-6">
                <div className="flex items-center gap-3">
                  <Droplet className={`w-5 h-5 ${getMoistureColor(device.moisture_lvl)}`} />
                  <div>
                    <div className="text-xs text-slate-500">Wilgotność</div>
                    <div className={`text-lg ${getMoistureColor(device.moisture_lvl)}`}>
                      {device.moisture_lvl !== null ? `${device.moisture_lvl}%` : '—'}
                    </div>
                  </div>
                </div>
                <div className="flex items-center gap-3">
                  <Battery className={`w-5 h-5 ${getBatteryColor(device.battery_lvl)}`} />
                  <div>
                    <div className="text-xs text-slate-500">Bateria</div>
                    <div className={`text-lg ${getBatteryColor(device.battery_lvl)}`}>
                      {device.battery_lvl !== null ? `${device.battery_lvl}%` : '—'}
                    </div>
                  </div>
                </div>
                <div className="flex items-center gap-3">
                  <Waves className={`w-5 h-5 ${getMoistureColor(device.water_lvl)}`} />
                  <div>
                    <div className="text-xs text-slate-500">Poziom wody</div>
                    <div className={`text-lg ${getMoistureColor(device.water_lvl)}`}>
                      {device.water_lvl !== null ? `${device.water_lvl}%` : '—'}
                    </div>
                  </div>
                </div>
              </div>

              <div className="flex justify-start">
                <button
                  onClick={(e) => handleForceWatering(e, device.device_id)}
                  className="flex items-center justify-center gap-2 px-5 py-2.5 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-lg hover:shadow transition-shadow"
                >
                  <Play className="w-4 h-4" />
                  Wymuś podlewanie
                </button>
              </div>
            </div>
          ))}
        </div>
      </div>

      <Dialog open={groupDialogOpen} onOpenChange={(open) => (open ? setGroupDialogOpen(true) : closeGroupDialog())}>
        <DialogContent className="sm:max-w-2xl bg-white/95 backdrop-blur-xl border-slate-200/70 shadow-2xl">
          <DialogHeader>
            <DialogTitle className="text-2xl text-slate-800">
              {activeGroup ? `Ustawienia grupy: ${activeGroup.name}` : 'Nowa grupa urządzeń'}
            </DialogTitle>
          </DialogHeader>

          <div className="space-y-6 pt-2">
            <div>
              <label className="block text-slate-700 mb-2">Nazwa grupy</label>
              <input
                type="text"
                value={groupName}
                onChange={(e) => setGroupName(e.target.value)}
                className="w-full px-4 py-3 border border-slate-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-emerald-500"
                placeholder="Wprowadź nazwę grupy"
                onKeyDown={(e) => e.key === 'Enter' && handleSaveGroup()}
              />
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              <div>
                <div className="flex justify-between items-center mb-2">
                  <label className="text-slate-700">Próg wilgotności gleby</label>
                  <span className="text-2xl text-blue-600">{groupMoistureThreshold}%</span>
                </div>
                <input
                  type="range"
                  min="0"
                  max="100"
                  value={groupMoistureThreshold}
                  onChange={(e) => setGroupMoistureThreshold(Number(e.target.value))}
                  className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer"
                />
              </div>
              <div>
                <div className="flex justify-between items-center mb-2">
                  <label className="text-slate-700">Czas podlewania</label>
                  <span className="text-2xl text-blue-600">{groupWateringDuration}s</span>
                </div>
                <input
                  type="range"
                  min="1"
                  max="60"
                  value={groupWateringDuration}
                  onChange={(e) => setGroupWateringDuration(Number(e.target.value))}
                  className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer"
                />
              </div>
            </div>

            <div>
              <label className="block text-slate-700 mb-3">Przypisane urządzenia</label>
              <div className="space-y-2 max-h-48 overflow-y-auto border border-slate-200 rounded-lg p-3 bg-slate-50">
                {devices.length === 0 ? (
                  <p className="text-sm text-slate-500">Brak dostępnych urządzeń</p>
                ) : (
                  devices.map((device) => (
                    <label key={device.device_id} className="flex items-center gap-3 p-2 hover:bg-slate-100 rounded cursor-pointer transition-colors">
                      <input
                        type="checkbox"
                        checked={groupSelectedDeviceIds.has(device.device_id)}
                        onChange={(e) => {
                          const newIds = new Set(groupSelectedDeviceIds);
                          if (e.target.checked) {
                            newIds.add(device.device_id);
                          } else {
                            newIds.delete(device.device_id);
                          }
                          setGroupSelectedDeviceIds(newIds);
                        }}
                        className="w-4 h-4 text-emerald-600 border-slate-300 rounded focus:ring-emerald-500"
                      />
                      <div className="min-w-0 flex-1">
                        <div className="text-sm font-medium text-slate-800 truncate">{device.name}</div>
                        <div className="text-xs text-slate-500 truncate">{device.device_id}</div>
                      </div>
                    </label>
                  ))
                )}
              </div>
              <p className="mt-2 text-sm text-slate-500">
                Zaznacz urządzenia, które mają należeć do tej grupy.
              </p>
            </div>

            <div className="flex flex-col-reverse md:flex-row md:items-center md:justify-between gap-3 pt-2">
              <div className="flex gap-3">
                {activeGroup && (
                  <button
                    type="button"
                    onClick={openDeleteConfirm}
                    disabled={savingGroup}
                    className="flex items-center gap-2 px-4 py-3 rounded-xl border border-red-200 text-red-600 hover:bg-red-50 transition-colors disabled:opacity-50"
                  >
                    <Trash2 className="w-4 h-4" />
                    Usuń grupę
                  </button>
                )}
              </div>
              <button
                onClick={handleSaveGroup}
                disabled={savingGroup || !groupName.trim()}
                className="flex items-center justify-center gap-3 px-6 py-4 bg-gradient-to-r from-emerald-500 to-teal-600 text-white rounded-xl hover:shadow-lg transition-shadow disabled:opacity-50 disabled:cursor-not-allowed"
              >
                {savingGroup ? 'Zapisywanie…' : activeGroup ? 'Zapisz zmiany' : 'Utwórz grupę'}
              </button>
            </div>
          </div>
        </DialogContent>
      </Dialog>

      <AlertDialog open={waterConfirmDeviceId !== null} onOpenChange={(open) => !open && setWaterConfirmDeviceId(null)}>
        <AlertDialogContent className="sm:max-w-md">
          <AlertDialogHeader>
            <AlertDialogTitle>Wymusić podlewanie?</AlertDialogTitle>
            <AlertDialogDescription>
              Ta akcja wyśle komendę podlewania do urządzenia {waterConfirmDeviceId}. Komunikat potwierdzenia zostanie pokazany w aplikacji.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <AlertDialogFooter>
            <AlertDialogCancel>Anuluj</AlertDialogCancel>
            <AlertDialogAction onClick={confirmForceWatering}>Wymuś podlewanie</AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog>

      <AlertDialog open={deleteConfirmOpen} onOpenChange={setDeleteConfirmOpen}>
        <AlertDialogContent className="sm:max-w-md">
          <AlertDialogHeader>
            <AlertDialogTitle>Usunąć grupę?</AlertDialogTitle>
            <AlertDialogDescription>
              Grupa {activeGroup?.name} zostanie usunięta. Urządzenia nie zostaną skasowane, ale stracą przypisanie do tej grupy.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <AlertDialogFooter>
            <AlertDialogCancel>Anuluj</AlertDialogCancel>
            <AlertDialogAction onClick={handleDeleteGroup}>Usuń grupę</AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog>
    </div>
  );
}
