import { Outlet, Link, useLocation } from "react-router";
import { Layers, Settings } from "lucide-react";

export default function Root() {
  const location = useLocation();

  const navigation = [
    { name: "Urządzenia", path: "/", icon: Layers },
    { name: "Ustawienia", path: "/settings", icon: Settings },
  ];

  const isActive = (path: string) => {
    if (path === "/") {
      return location.pathname === "/";
    }
    return location.pathname.startsWith(path);
  };

  return (
    <div className="size-full min-h-screen from-slate-50 to-slate-100">
      {/* Navigation */}
      <nav className="bg-white border-b border-slate-200 shadow-sm">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between h-16">
            <div className="flex">
              <div className="flex-shrink-0 flex items-center">
                <div className="w-8 h-8 bg-gradient-to-br from-emerald-500 to-teal-600 rounded-lg"></div>
                <span className="ml-3 text-xl text-slate-800">Panel Monitoringu</span>
              </div>
              <div className="hidden sm:ml-8 sm:flex sm:space-x-4">
                {navigation.map((item) => {
                  const Icon = item.icon;
                  const active = isActive(item.path);
                  return (
                    <Link
                      key={item.path}
                      to={item.path}
                      className={`inline-flex items-center px-4 py-2 rounded-lg transition-colors ${
                        active
                          ? "bg-emerald-50 text-emerald-700"
                          : "text-slate-600 hover:bg-slate-50 hover:text-slate-900"
                      }`}
                    >
                      <Icon className="w-4 h-4 mr-2" />
                      {item.name}
                    </Link>
                  );
                })}
              </div>
            </div>
            
            <div className="hidden sm:ml-8 sm:flex sm:items-center">
              <button 
                onClick={() => {
                  localStorage.removeItem('access_token');
                  window.location.href = '/login';
                }}
                className="text-slate-600 hover:text-red-600 font-medium px-3 py-2 rounded-lg transition-colors"
              >
                Wyloguj
              </button>
            </div>
          </div>
        </div>

        {/* Mobile Navigation */}
        <div className="sm:hidden border-t border-slate-200">
          <div className="flex justify-around py-2">
            {navigation.map((item) => {
              const Icon = item.icon;
              const active = isActive(item.path);
              return (
                <Link
                  key={item.path}
                  to={item.path}
                  className={`flex flex-col items-center px-3 py-2 rounded-lg transition-colors ${
                    active
                      ? "text-emerald-700"
                      : "text-slate-600"
                  }`}
                >
                  <Icon className="w-5 h-5" />
                  <span className="text-xs mt-1">{item.name}</span>
                </Link>
              );
            })}
          </div>
        </div>
      </nav>

      {/* Page Content */}
      <main>
        <Outlet />
      </main>
    </div>
  );
}
