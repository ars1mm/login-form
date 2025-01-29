'use client'

import { useEffect, useState } from 'react'
import { useRouter } from 'next/navigation'

const API_BASE_URL = 'http://localhost:8080'

export default function AdminPanel() {
  const router = useRouter()
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    const verifyAdmin = async () => {
      try {
        const token = sessionStorage.getItem('token')
        const isAdmin = sessionStorage.getItem('isAdmin')

        if (!token || isAdmin !== 'true') {
          throw new Error('Not authorized')
        }

        const response = await fetch(`${API_BASE_URL}/verify`, {
          method: 'GET',
          headers: {
            'Authorization': `Bearer ${token}`,
            'Content-Type': 'application/json'
          }
        })

        const data = await response.json()
        
        if (!response.ok || !data.success) {
          throw new Error(data.message || 'Session verification failed')
        }

        setLoading(false)
      } catch (err) {
        console.error('Session verification failed:', err)
        sessionStorage.clear()
        router.push('/')
      }
    }

    verifyAdmin()
  }, [router])

  if (loading) {
    return <div className="min-h-screen bg-gray-100 flex items-center justify-center">
      <p>Loading...</p>
    </div>
  }

  return (
    <main className="min-h-screen bg-gray-100">
      <nav className="bg-white shadow-md p-4">
        <div className="container mx-auto flex justify-between items-center">
          <h1 className="text-xl font-bold">Admin Panel</h1>
          <div className="flex items-center gap-4">
            <span>Welcome {sessionStorage.getItem('username')}</span>
            <button
              onClick={() => {
                sessionStorage.clear()
                router.push('/')
              }}
              className="bg-red-500 text-white px-4 py-2 rounded hover:bg-red-600 transition-colors"
            >
              Logout
            </button>
          </div>
        </div>
      </nav>
      <div className="container mx-auto p-4">
        <h2 className="text-2xl font-bold mb-4">Admin Dashboard</h2>
        {/* Add admin features here */}
      </div>
    </main>
  )
}
