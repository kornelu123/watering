import { createBrowserRouter } from "react-router";
import Root from "./pages/Root";
import DeviceList from "./pages/DeviceList";
import DeviceAnalytics from "./pages/DeviceAnalytics";
import DeviceSettings from "./pages/DeviceSettings";
import AddDevice from "./pages/AddDevice";
import Settings from "./pages/Settings";
import NotFound from "./pages/NotFound";
import Login from "./pages/Login";
import Register from "./pages/Register";
import ProtectedRoute from "./components/ProtectedRoute";

export const router = createBrowserRouter([
  { path: "/login", Component: Login },
  { path: "/register", Component: Register },
  {
    path: "/",
    Component: ProtectedRoute,
    children: [
      {
        path: "/",
        Component: Root,
        children: [
          { index: true, Component: DeviceList },
          { path: "devices", Component: DeviceList },
          { path: "devices/new", Component: AddDevice },
          { path: "devices/:deviceId", Component: DeviceAnalytics },
          { path: "devices/:deviceId/settings", Component: DeviceSettings },
          { path: "settings", Component: Settings },
          { path: "*", Component: NotFound },
        ],
      }
    ]
  },
]);
