'use client'

import { useState } from 'react'
import { useRouter } from 'next/navigation'

const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:8080'

export default function Home() {
  const [isLogin, setIsLogin] = useState(true)
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState('')
  const router = useRouter()

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    setError('')
    const endpoint = isLogin ? '/login' : '/signup'

    try {
      const response = await fetch(`${API_BASE_URL}${endpoint}`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        credentials: 'same-origin',
        body: JSON.stringify({ username, password })
      })

      // Get raw response first
      const rawResponse = await response.text()
      console.log('Raw response:', rawResponse)

      // Try to parse as JSON
      let data
      try {
        data = JSON.parse(rawResponse)
        console.log('Parsed response:', data)
      } catch (parseError) {
        console.error('JSON Parse Error:', parseError)
        throw new Error('Server returned invalid JSON')
      }

      // Check response structure
      if (!data || typeof data.success !== 'boolean') {
        console.error('Invalid response structure:', data)
        throw new Error('Invalid response format from server')
      }

      // Handle success case
      if (data.success) {
        console.log('Login successful:', data)
        sessionStorage.setItem('token', data.token || '')
        sessionStorage.setItem('username', username)
        sessionStorage.setItem('isAdmin', String(Boolean(data.is_admin)))
        
        if (data.is_admin) {
          router.push('/admin')
        } else {
          router.push('/dashboard')
        }
      } else {
        // Handle error case
        throw new Error(data.message || 'Authentication failed')
      }

    } catch (err: any) {
      console.error('Full error details:', err)
      setError(err.message || 'Failed to connect to server')
    }
  }

  return (
    <main className="min-h-screen bg-gray-100 flex items-center justify-center p-4">
      <div className="bg-white p-8 rounded-lg shadow-md w-full max-w-md">
        <div className="flex justify-between mb-6">
          <button 
            onClick={() => setIsLogin(true)}
            className={`text-lg font-semibold ${isLogin ? 'text-blue-500' : 'text-gray-500'}`}
          >
            Login
          </button>
          <button 
            onClick={() => setIsLogin(false)}
            className={`text-lg font-semibold ${!isLogin ? 'text-blue-500' : 'text-gray-500'}`}
          >
            Sign Up
          </button>
        </div>

        {error && (
          <div className="mb-4 text-red-500 text-sm">{error}</div>
        )}
        
        <form onSubmit={handleSubmit} className="space-y-4">
          <input
            type="text"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            placeholder="Username"
            className="w-full p-2 border rounded focus:outline-none focus:ring-2 focus:ring-blue-500"
            required
          />
          
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="Password"
            className="w-full p-2 border rounded focus:outline-none focus:ring-2 focus:ring-blue-500"
            required
          />
          
          <button
            type="submit"
            className="w-full bg-blue-500 text-white p-2 rounded hover:bg-blue-600 transition-colors"
          >
            {isLogin ? 'Login' : 'Sign Up'}
          </button>
        </form>
      </div>
    </main>
  )
}
