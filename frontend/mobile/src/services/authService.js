import axios from 'axios';

const API_BASE_URL = process.env.API_BASE_URL || 'http://192.168.1.123:8000';

const apiClient = axios.create({
  baseURL: `${API_BASE_URL}/api/auth`,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
});

export const authService = {
  async register(name, family_name, email, password, repeat_password) {
    try {
      const response = await apiClient.post('/register', {
        name,
        family_name,
        email,
        password,
        repeat_password,
      });
      return response.data;
    } catch (error) {
      if (error.response && error.response.data) {
        return { success: false, error: error.response.data.error };
      }
      return { success: false, error: 'Network error' };
    }
  },

  async login(email, password) {
    try {
      const response = await apiClient.post('/login', {
        email,
        password,
      });
      return response.data;
    } catch (error) {
      if (error.response && error.response.data) {
        return { success: false, error: error.response.data.error };
      }
      return { success: false, error: 'Network error' };
    }
  },

  async verifyToken(token) {
    try {
      const response = await apiClient.post('/verify', {}, {
        headers: {
          Authorization: `Bearer ${token}`,
        },
      });
      return response.data;
    } catch (error) {
      return { success: false, error: 'Token verification failed' };
    }
  },

  async logout(token) {
    try {
      const response = await apiClient.post('/logout', {}, {
        headers: {
          Authorization: `Bearer ${token}`,
        },
      });
      return response.data;
    } catch (error) {
      return { success: false, error: 'Logout failed' };
    }
  },
};