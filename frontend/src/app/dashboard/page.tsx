'use client'

import { useEffect } from 'react'
import { useRouter } from 'next/navigation'

export default function Dashboard() {
  const router = useRouter()

  useEffect(() => {
    const token = sessionStorage.getItem('token')
    const isAdmin = sessionStorage.getItem('isAdmin')

    // Redirect if no token or if admin
    if (!token || isAdmin === 'true') {
      router.push('/')
    }
  }, [router])

  return (
    <main className="min-h-screen bg-gray-100">
      <nav className="bg-white shadow-md p-4">
        <div className="container mx-auto flex justify-between items-center">
          <h1 className="text-xl font-bold">
            Welcome {sessionStorage.getItem('username')}
          </h1>
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
      </nav>
    </main>
  )
}
