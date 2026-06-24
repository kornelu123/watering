import { Link } from "react-router";
import { Home, ArrowLeft } from "lucide-react";

export default function NotFound() {
  return (
    <div className="p-8">
      <div className="max-w-2xl mx-auto text-center mt-20">
        <div className="mb-8">
          <div className="text-9xl mb-4">404</div>
          <h1 className="text-4xl mb-4 text-slate-800">Strona nie znaleziona</h1>
          <p className="text-slate-600 text-lg mb-8">
            Przepraszamy, ale strona której szukasz nie istnieje.
          </p>
        </div>

        <div className="flex gap-4 justify-center">
          <Link
            to="/"
            className="flex items-center gap-2 px-6 py-3 bg-gradient-to-r from-emerald-500 to-teal-600 text-white rounded-xl hover:shadow-lg transition-shadow"
          >
            <Home className="w-5 h-5" />
            Strona główna
          </Link>
          <button
            onClick={() => window.history.back()}
            className="flex items-center gap-2 px-6 py-3 bg-white text-slate-700 border border-slate-300 rounded-xl hover:bg-slate-50 transition-colors"
          >
            <ArrowLeft className="w-5 h-5" />
            Wróć
          </button>
        </div>
      </div>
    </div>
  );
}
