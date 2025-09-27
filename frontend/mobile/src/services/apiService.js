import axios from 'axios';
import AsyncStorage from '@react-native-async-storage/async-storage';

const API_BASE_URL = process.env.API_BASE_URL || 'http://192.168.1.123:8000';

// Create axios instance
const apiClient = axios.create({
  baseURL: `${API_BASE_URL}/api`,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Add auth token to requests
apiClient.interceptors.request.use(async (config) => {
  const token = await AsyncStorage.getItem('userToken');
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

// Handle auth errors
apiClient.interceptors.response.use(
  (response) => response,
  async (error) => {
    if (error.response?.status === 401) {
      await AsyncStorage.removeItem('userToken');
      await AsyncStorage.removeItem('user');
      // Navigate to login would be handled by auth context
    }
    return Promise.reject(error);
  }
);

export const eventsAPI = {
  // Get all user events
  async getEvents() {
    try {
      const response = await apiClient.get('/bills/events');
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Create new event
  async createEvent(eventData) {
    try {
      const response = await apiClient.post('/bills/events', eventData);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Get event details
  async getEvent(eventId) {
    try {
      const response = await apiClient.get(`/bills/events/${eventId}`);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Get event expenses
  async getExpenses(eventId) {
    try {
      const response = await apiClient.get(`/bills/events/${eventId}/expenses`);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Add expense
  async addExpense(eventId, expenseData) {
    try {
      const response = await apiClient.post(`/bills/events/${eventId}/expenses`, expenseData);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Get participants
  async getParticipants(eventId) {
    try {
      const response = await apiClient.get(`/bills/events/${eventId}/participants`);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Add participant
  async addParticipant(eventId, participantData) {
    try {
      const response = await apiClient.post(`/bills/events/${eventId}/participants`, participantData);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Get settlements
  async getSettlements(eventId) {
    try {
      const response = await apiClient.get(`/bills/events/${eventId}/settlements`);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Record payment
  async recordPayment(eventId, paymentData) {
    try {
      const response = await apiClient.post(`/bills/events/${eventId}/payments`, paymentData);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },
};

export const userAPI = {
  // Get user balance
  async getBalance() {
    try {
      const response = await apiClient.get('/bills/users/balance');
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },
};

export const contactsAPI = {
  // Search users
  async searchUsers(query) {
    try {
      const response = await apiClient.get(`/contacts/users/search?query=${encodeURIComponent(query)}`);
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },

  // Get contacts
  async getContacts() {
    try {
      const response = await apiClient.get('/contacts/contacts');
      return { success: true, data: response.data };
    } catch (error) {
      return { success: false, error: error.response?.data?.error || 'Network error' };
    }
  },
};