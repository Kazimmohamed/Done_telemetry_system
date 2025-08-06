import { useState } from 'react'
import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import './App.css'

function App() {
  const [count, setCount] = useState(0)

  return (
    <>
      <div className="flex justify-center items-center space-x-4 my-6">
        <a href="https://vite.dev" target="_blank">
          <img src={viteLogo} className="w-24 h-24" alt="Vite logo" />
        </a>
        <a href="https://react.dev" target="_blank">
          <img src={reactLogo} className="w-24 h-24" alt="React logo" />
        </a>
      </div>
      <h1 className="text-4xl font-bold text-center text-indigo-600 mb-4">Vite + React + Tailwind</h1>
      <div className="card text-center">
        <button
          className="px-4 py-2 bg-blue-500 hover:bg-blue-700 text-white rounded-md mb-2"
          onClick={() => setCount((count) => count + 1)}
        >
          count is {count}
        </button>
        <p className="text-gray-600">
          Edit <code>src/App.jsx</code> and save to test HMR
        </p>
      </div>
      <p className="text-center text-gray-500 mt-4">
        Click on the Vite and React logos to learn more
      </p>
    </>
  )
}

export default App

